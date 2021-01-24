/*
 * PHY implementation covert channel
 * Responsible for sending a bit over the cache covert channel 
 * and management for evictions/probe lists.
 * 
 * Copyright (C) 2021 Ursescu Ionut 
 */

#include <iostream>
#include <cmath>
#include <bitset>
#include <chrono>

#include "phy.hh"

using namespace std::chrono;

/*
    Flushes all caches of the input address.
*/
static void clflush(uintptr_t addr)
{
	asm volatile("clflush (%0)" ::"r"(addr));
}

/* Measure the time it takes to access a block with virtual address addr. 
 * (https://stackoverflow.com/questions/21369381/measuring-cache-latencies)
 */
static uint64_t measure_acces_time(uintptr_t addr)
{
	uint64_t cycles;

	asm volatile("mov %1, %%r8\n\t"
		     "lfence\n\t"
		     "rdtsc\n\t"
		     "mov %%eax, %%edi\n\t"
		     "mov (%%r8), %%r8\n\t"
		     "lfence\n\t"
		     "rdtsc\n\t"
		     "sub %%edi, %%eax\n\t"
		     : "=a"(cycles) /*output*/
		     : "r"(addr)
		     : "r8", "edi");

	return cycles;
}

static int64_t time_diff(time_point<system_clock> current)
{
	return duration_cast<microseconds>(system_clock::now() - current)
		.count();
}

/*
 * Returns the cache l3 set index of a given address;
 */
static uint64_t cache_l3_set_index(uintptr_t addr)
{
	uint64_t mask =
		((uint64_t)1 << ((uint32_t)log2(CacheConfig::CACHE_L3_SETS) +
				 (uint32_t)log2(CacheConfig::CACHE_LINESIZE))) -
		1;
	return (addr & mask) >> (uint32_t)log2(CacheConfig::CACHE_LINESIZE);
}

/*
 * Returns the cache l1 set index of a given address.
 */
static uint64_t cache_l1_set_index(uintptr_t addr)
{
	uint64_t mask =
		((uint64_t)1 << ((uint32_t)log2(CacheConfig::CACHE_L1_SETS) +
				 (uint32_t)log2(CacheConfig::CACHE_LINESIZE))) -
		1;
	return (addr & mask) >> (uint32_t)log2(CacheConfig::CACHE_LINESIZE);
}

/*
 * Build the the buffer and the list of addresses used to read the data.
 */
static void build_probe_list(PhyConfig &config, uint32_t set_number)
{
	uint32_t line_offsets = log2(CacheConfig::CACHE_LINESIZE);
	uint32_t sets = log2(CacheConfig::CACHE_L1_SETS);

	// Create a buffer of at least as large as the L1 cache
	uint32_t buffer_size =
		CacheConfig::CACHE_L1_WAYS * exp2(line_offsets + sets);

	config.probeBuffer = (uint8_t *)malloc(buffer_size);

	for (uint32_t i = 0; i < CacheConfig::CACHE_L1_WAYS * exp2(sets); i++) {
		uint32_t idx = i * exp2(line_offsets);

		uintptr_t addr = (uintptr_t) & (config.probeBuffer[idx]);

		if (cache_l1_set_index(addr) == set_number) {
			// Only addresses from cache set 0 in L1
			config.probe_list.emplace_back(addr);
		}
	}
}

/*
 *  Build the buffer and the list of addresses used to evict cache
 */
static void build_eviction_list(PhyConfig &config, uint32_t set_number)
{
	uint32_t line_offsets = log2(CacheConfig::CACHE_LINESIZE);
	uint32_t sets = log2(CacheConfig::CACHE_L3_SETS);

	// Create buffer at least as large as the L3 cache
	uint32_t buffer_size = CacheConfig::CACHE_L3_WAYS *
			       CacheConfig::CACHE_LINESIZE *
			       CacheConfig::CACHE_L3_SETS;
	config.evictBuffer = (uint8_t *)malloc(buffer_size);

	// Search for an address from Cache set 0
	for (uint32_t i = 0; i < CacheConfig::CACHE_L3_SETS; i++) {
		for (uint32_t j = 0; j < CacheConfig::CACHE_L3_WAYS; j++) {
			uint32_t idx =
				(uint32_t)(i * exp2(line_offsets)) +
				(uint32_t)(j * exp2(line_offsets + sets));
			uintptr_t addr =
				(uintptr_t) & (config.evictBuffer[idx]);

			// Focus fire on a single cache set.  In particular, cache set 0.
			if (cache_l3_set_index(addr) == set_number) {
				config.eviction_list.emplace_back(addr);
			}
		}
	}
}

/*
 * Listen for bit, this should listen for it at specific timestamp
 */
BIT phy_recv(PhyConfig &config)
{
	uint32_t access_count = 0U;
	uint32_t cache_hit = 0U;
	uint32_t cache_miss = 0U;
	double hit_ratio = -1;

	clock_t start = clock();
	clock_t dt = clock() - start;

	auto current = system_clock::now();

	while (time_diff(current) < config.period) {
		for (auto it = config.probe_list.begin();
		     it != config.probe_list.end(); it++) {
			uintptr_t addr = (uintptr_t)(*it);
			uint64_t cycles = measure_acces_time(addr);

			if (cycles > 1000)
				continue;

			access_count++;

			if (cycles <= config.miss_time)
				cache_hit++;
			else
				cache_miss++;

			// (TUNE) lag time for sender to flush L3 -- TODO check if really necessary in the lastest update
			for (uint32_t j = 0;
			     j < (config.period * 0.02) &&
			     (time_diff(current) < config.period);
			     j++) {
				// do nothing
			}
		}
	}

	// Return based on hit ratio
	hit_ratio = (double)cache_hit / (double)access_count;
	// std::cout << "Hit ratio " << hit_ratio << std::endl;
	return (hit_ratio < 0.5) ? BIT_1 : BIT_0;
}

/*
	Send a zero bit to the receiver through the covert channel.  
		To send zero, the cache is NOT flushed.  
		Stalls for configuration.period microseconds.  
*/
static void send_zero(PhyConfig &config)
{
	auto current = system_clock::now();

	while (time_diff(current) < config.period) {
		; // Just loop here
	}
}

/*
 * Send a one bit to the receiver through the covert channel.  
 * To send one, the entire LLC cache is flushed.  
 * Flush until configuration.period microseconds has passed.  
 */
static void send_one(PhyConfig &config)
{
	auto current = system_clock::now();

	while (time_diff(current) < config.period) {
		for (auto it = config.eviction_list.begin();
		     it != config.eviction_list.end(); it++) {
			uintptr_t addr = (uintptr_t)(*it);
			clflush(addr);
		}
	}
}

/* 
 * Wrapper for send bit
 */
void phy_send(PhyConfig &config, BIT bit)
{
	if (bit == 0U) {
		send_zero(config);
	} else {
		send_one(config);
	}
}

/*
 * Function that will build the eviction/probing sets.
 */
void init_phy_dev(PhyConfig &config, uint32_t send_channel, uint32_t recv_channel) {

	/* Build eviction set */
	build_eviction_list(config, send_channel);

	/* Build the recv probe list */
	build_probe_list(config, recv_channel);
}