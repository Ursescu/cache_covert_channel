#include <iostream>
#include <cmath>
#include <bitset>
#include <time.h>

#include "covert.hh"

/* Init static sequence numbers */

std::vector<BIT> ProtocolConfig::startSequence = { 1, 0, 1, 1, 0, 1, 1 };
std::vector<BIT> ProtocolConfig::endSequence = { 0, 0, 0, 0, 0, 0, 0, 0 };

/*
    Flushes all caches of the input address.  - From linux kernel
*/
void clflush(uintptr_t addr)
{
	asm volatile("clflush (%0)" ::"r"(addr));
}

/* Measure the time it takes to access a block with virtual address addr. 
 * (https://stackoverflow.com/questions/21369381/measuring-cache-latencies)
 */
uint64_t measure_acces_time(uintptr_t addr)
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

/*
 * Returns the cache l3 set index of a given address;
 */
uint64_t cache_l3_set_index(uintptr_t addr)
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
uint64_t cache_l1_set_index(uintptr_t addr)
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
void build_probe_list(ReceiverConfig &configuration)
{
	uint32_t line_offsets = log2(CacheConfig::CACHE_LINESIZE);
	uint32_t sets = log2(CacheConfig::CACHE_L1_SETS);

	// Create a buffer of at least as large as the L1 cache
	uint32_t buffer_size =
		CacheConfig::CACHE_L1_WAYS * exp2(line_offsets + sets);

	configuration.buffer = (uint8_t *)malloc(buffer_size);

	std::cout << std::bitset<sizeof(uintptr_t) * 8>(
			     (uintptr_t) & (configuration.buffer[0]))
		  << std::endl;

	for (uint32_t i = 0; i < CacheConfig::CACHE_L1_WAYS * exp2(sets); i++) {
		uint32_t idx = i * exp2(line_offsets);

		uintptr_t addr = (uintptr_t) & (configuration.buffer[idx]);

		if (cache_l1_set_index(addr) == 0x0) {
			// Only addresses from cache set 0 in L1
			configuration.probe_list.emplace_back(addr);
		}
	}
}

/*
 *  Build the buffer and the list of addresses used to evict cache
 */
void build_eviction_list(SenderConfig &configuration)
{
	uint32_t line_offsets = log2(CacheConfig::CACHE_LINESIZE);
	uint32_t sets = log2(CacheConfig::CACHE_L3_SETS);

	// Create buffer at least as large as the L3 cache
	uint32_t buffer_size = CacheConfig::CACHE_L3_WAYS *
			       CacheConfig::CACHE_LINESIZE *
			       CacheConfig::CACHE_L3_SETS;
	configuration.buffer = (uint8_t *)malloc(buffer_size);

	// Search for an address from Cache set 0
	for (uint32_t i = 0; i < CacheConfig::CACHE_L3_SETS; i++) {
		for (uint32_t j = 0; j < CacheConfig::CACHE_L3_WAYS; j++) {
			uint32_t idx =
				(uint32_t)(i * exp2(line_offsets)) +
				(uint32_t)(j * exp2(line_offsets + sets));
			uintptr_t addr =
				(uintptr_t) & (configuration.buffer[idx]);

			// Focus fire on a single cache set.  In particular, cache set 0.
			if (cache_l3_set_index(addr) == 0x0) {
				configuration.eviction_list.emplace_back(addr);
			}
		}
	}
}

void time_correction(ReceiverConfig &configuration, uint32_t misses,
		     uint32_t total)
{
	clock_t start = clock();

	double H = (double)total * 0.75;
	double out_of_sync = (1 - misses / H);

	double pause_time = configuration.period * out_of_sync;

	while ((clock() - start) < pause_time) {
		// do nothing
	}

	return;
}

/*
 * Listen for bit
 */
uint8_t listen_for_bit(ReceiverConfig &configuration)
{
	uint32_t access_count = 0U;
	uint32_t cache_hit = 0U;
	uint32_t cache_miss = 0U;
	int32_t hit_ratio = -1;

	clock_t start = clock();
	clock_t dt = clock() - start;

	while (dt < configuration.period) {
		for (auto it = configuration.probe_list.begin();
		     it != configuration.probe_list.end(); it++) {
			uintptr_t addr = (uintptr_t)(*it);
			uint64_t cycles = measure_acces_time(addr);

			if (cycles > 1000)
				continue;

			access_count++;
			// std::cout << "Cycles " << cycles << std::endl;
			if (cycles <= configuration.miss_time)
				cache_hit++;
			else
				cache_miss++;

			// (TUNE) lag time for sender to flush L3
			for (uint32_t j = 0;
			     j < (configuration.period * 0.02) &&
			     (clock() - start) < configuration.period;
			     j++) {
				// do nothing
			}
		}
		dt = clock() - start;
	}

	/* HACK - NEED TO UNDERSTAND WHY IT'S NOT WORKING */
	if (cache_miss > (access_count * 0.20))
		time_correction(configuration, cache_miss, access_count);

	if (cache_hit < ((double)access_count * 0.8))
		return 1;
	else
		return 0;

	// Return based on hit ratio
	hit_ratio = (double)cache_hit / (double)access_count;
	return (hit_ratio < 0.5) ? 1U : 0U;
}

/*
	Send a zero bit to the receiver through the covert channel.  
		To send zero, the cache is NOT flushed.  
		Stalls for configuration.period microseconds.  
*/
void send_zero(SenderConfig &configuration)
{
	clock_t start;
	start = clock();

	while (clock() - start < configuration.period) {
		// wait
	}
}

/*
	Send a one bit to the receiver through the covert channel.  
		To send one, the entire LLC cache is flushed.  
		Flush until configuration.period microseconds has passed.  
*/
void send_one(SenderConfig &configuration)
{
	clock_t start = clock();
	clock_t dt = clock() - start;

	while (dt < configuration.period) {
		for (auto it = configuration.eviction_list.begin();
		     it != configuration.eviction_list.end(); it++) {
			uintptr_t addr = (uintptr_t)(*it);
			clflush(addr);
		}

		dt = clock() - start;
	}
}