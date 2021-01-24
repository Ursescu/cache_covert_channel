#ifndef _LL_HH_
#define _LL_HH_

#include <cstdint>
#include <thread>
#include <vector>
#include "CRC.h"

#include "queue-threadsafe.hpp"
#include "phy.hh"


#define BYTE_SIZE (sizeof(uint8_t) * 8U)

struct LLConfig {
	static const uint32_t MAX_SDU_LEN = 10U;
	static const uint32_t MIN_PDU_LEN = 3U;
	static const uint32_t MAX_PDU_LEN = MAX_SDU_LEN + MIN_PDU_LEN;
	static const uint32_t MAX_FRAME_BIT_LEN = (MAX_PDU_LEN + 2U) * BYTE_SIZE;
	static const uint32_t RETRIES = 5U;
	static const uint32_t SYNC_US = 10000U;
	static const uint32_t SYNC_MASK = 100000U;
	static const uint32_t MAX_TX_QUEUE_SIZE = 20U;
	static const uint32_t MAX_RX_QUEUE_SIZE = 20U;

	static std::vector<BIT> start_seq;
	static std::vector<BIT> end_seq;
};

enum pdu_type {
	PDU_DATA_REQ = 0x00,
	PDU_DATA,
	PDU_ACK,
	PDU_NACK,
	/* Used to kill threads */
	PDU_EXIT,
};

struct ll_pdu {
	uint8_t seq;
	pdu_type type;
	uint8_t crc;
	std::vector<uint8_t> data;

	ll_pdu() = default;
	ll_pdu(std::vector<uint8_t> &data, uint8_t seq);
	ll_pdu(std::vector<uint8_t> &data, uint8_t seq, uint8_t crc);
};

/* Main structure that will keep track of link layer device informations */
struct ll_dev {

	std::threadsafe::queue<ll_pdu> rx_queue;
	std::threadsafe::queue<ll_pdu> tx_queue;

	PhyConfig phy_config;
	uint8_t curr_seq;

	std::thread rx_thread;
	std::thread tx_thread;


	ll_dev() = default;
};

extern void init_ll_dev(ll_dev &dev, uint32_t send_channel, uint32_t recv_channel);

/* Start ll */
extern void lll_run(ll_dev &dev);

/* Stop ll */
extern void lll_stop(ll_dev &dev);

/* Wait for ll to exit */
extern void lll_wait_exit(ll_dev &dev);

extern bool lll_send(ll_dev &dev, ll_pdu &pdu);

extern bool lll_recv(ll_dev &dev, ll_pdu &pdu);

#endif
