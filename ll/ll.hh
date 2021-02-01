#ifndef _LL_HH_
#define _LL_HH_

#include <cstdint>
#include <thread>
#include <vector>
#include <future>

#include "CRC.h"

#include "queue-threadsafe.hpp"
#include "phy.hh"

#define BYTE_SIZE (sizeof(uint8_t) * 8U)

struct LLConfig {
	
	static const uint32_t MAX_SDU_LEN = 3U;
	static const uint32_t MIN_PDU_LEN = 3U;
	static const uint32_t MAX_PDU_LEN = MAX_SDU_LEN + MIN_PDU_LEN;
	static const uint32_t MAX_FRAME_BIT_LEN = (MAX_PDU_LEN + 2U) * BYTE_SIZE;

	static const uint32_t SYNC_US = 10000U;
	static const uint32_t SYNC_MASK = 100000U;

	static const uint32_t RETRY_TIMEOUT_US = 200000U;

	static const uint32_t RETRIES = 5U;
	static const uint32_t WINDOW_SIZE = 2U;
	static const uint32_t SEGMENT_SIZE = WINDOW_SIZE * 3U;

	static const uint32_t QUEUE_SIZE = 30U;
	static const uint32_t LLL_TX_QUEUE_SIZE = QUEUE_SIZE;
	static const uint32_t LLL_RX_QUEUE_SIZE = QUEUE_SIZE;

	static const uint32_t ULL_RX_QUEUE_SIZE = QUEUE_SIZE;
	static const uint32_t ULL_TX_QUEUE_SIZE = QUEUE_SIZE;


	static std::vector<BIT> start_seq;
	static std::vector<BIT> end_seq;
};

enum pdu_type {
	PDU_DATA = 0x00,
	PDU_ACK,
	PDU_NACK,
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

	/* Transmission */
	uint8_t tx_seq;
	uint8_t rx_seq;


	

	/* LLL */
	std::threadsafe::queue<ll_pdu> lll_rx_queue;
	std::threadsafe::queue<ll_pdu> lll_tx_queue;
	
	std::thread lll_rx_thread;
	std::thread lll_tx_thread;
	std::promise<void> lll_tx_thread_signal;
	std::promise<void> lll_rx_thread_signal;

	/* ULL */
	std::threadsafe::queue<ll_pdu> ull_rx_queue;
	std::threadsafe::queue<ll_pdu> ull_tx_queue;

	std::thread ull_thread;
	std::promise<void> ull_thread_signal;
	
	/* Phy */
	PhyConfig phy_config;

	ll_dev() = default;
};

/* Init function also start the link layer threads */
extern void ll_init(ll_dev &dev, uint32_t send_channel, uint32_t recv_channel);

/* Kill ll state machine */
extern void ll_clear(ll_dev &dev);

/* Send/recv */
extern bool ll_send(ll_dev &dev, std::vector<uint8_t> &data);

extern bool ll_recv(ll_dev &dev, std::vector<uint8_t> &data);

/* Utils */
extern uint8_t ll_pdu_crc(ll_pdu &pdu);

#endif
