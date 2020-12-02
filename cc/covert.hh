#ifndef _COVERT_HH_
#define _COVERT_HH_

#include <vector>

// Cache configuration struct for i7 6700HQ
struct CacheConfig {
	static const uint32_t CACHE_LINESIZE = 64;
	static const uint32_t CACHE_L1_SETS = 64;
	static const uint32_t CACHE_L1_WAYS = 8;
	static const uint32_t CACHE_L1_SIZE =
		CACHE_L1_SETS * CACHE_L1_WAYS * CACHE_LINESIZE;
	static const uint32_t CACHE_L2_SETS = 1024;
	static const uint32_t CACHE_L2_WAYS = 4;
	static const uint32_t CACHE_L2_SIZE =
		CACHE_L2_SETS * CACHE_L2_WAYS * CACHE_LINESIZE;
	static const uint32_t CACHE_L3_SETS = 8192;
	static const uint32_t CACHE_L3_WAYS = 12;
	static const uint32_t CACHE_L3_SIZE =
		CACHE_L3_SETS * CACHE_L2_WAYS * CACHE_LINESIZE;
	static const uint32_t CACHE_L3_SLICES = 8;
	static const uint32_t PHYSC_CORES = 4;
	static const uint32_t POLLING_PERIOD = 1000;
	static const uint32_t MISS_TIME = 80;
	// Hyper threading
	static const uint32_t VIRTUAL_CORES = PHYSC_CORES * 2;
};

typedef uint8_t BIT;
typedef uint8_t BYTE;

/* Temporary config, needs to be implemented in JS */
struct ProtocolConfig {
	static std::vector<BIT> startSequence;
	static std::vector<BIT> endSequence;
};

// Config used to in receiving data
struct ReceiverConfig {
	// Buffer to probe
	uint8_t *buffer;

	// List of addresses to probe from cache set 0
	std::vector<uintptr_t> probe_list;

	// Period to measure cache access
	uint32_t period = CacheConfig::POLLING_PERIOD;

	// Miss time
	uint32_t miss_time = CacheConfig::MISS_TIME;

	uint32_t mode;
};

// Config used to send data
struct SenderConfig {
	// Buffer to evict set
	uint8_t *buffer;

	// List of address to evict cache set 1
	std::vector<uintptr_t> eviction_list;

	// Period to measure cache access
	uint32_t period = CacheConfig::POLLING_PERIOD;
};

extern uint64_t cache_l3_set_index(uintptr_t addr);
extern uint64_t cache_l1_set_index(uintptr_t addr);

extern void build_probe_list(ReceiverConfig &config);
extern void build_eviction_list(SenderConfig &config);

extern uint8_t listen_for_bit(ReceiverConfig &configuration);

extern void send_one(SenderConfig &config);
extern void send_zero(SenderConfig &config);

#endif