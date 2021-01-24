#ifndef _PHY_HH_
#define _PHY_HH_

#include <vector>
#include <cstdint>

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
	static const uint32_t POLLING_PERIOD = 500;
	static const uint32_t MISS_TIME = 80;
    /* Channels (aka cache sets) */
    static const uint32_t CHANNEL_0 = 0;
    static const uint32_t CHANNEL_1 = 1;
    static const uint32_t CHANNEL_2 = 2;
	// Hyper threading
	static const uint32_t VIRTUAL_CORES = PHYSC_CORES * 2;
};

typedef uint8_t BIT;
typedef uint8_t BYTE;

#define BIT_0 (0U)
#define BIT_1 (1U)

struct PhyConfig {

	uint8_t *probeBuffer;
	uint8_t *evictBuffer;

	// List of addresses to probe
	std::vector<uintptr_t> probe_list;
	// List of addresses to evict
	std::vector<uintptr_t> eviction_list;

	// Period to measure cache access
	uint32_t period = CacheConfig::POLLING_PERIOD;

	// Miss time
	uint32_t miss_time = CacheConfig::MISS_TIME;
};


extern void init_phy_dev(PhyConfig &config, uint32_t send_channel, uint32_t recv_channel);

extern BIT phy_recv(PhyConfig &config);

extern void phy_send(PhyConfig &config, BIT bit);

#endif