/*
 * Link layer implementation covert channel
 * Responsible for wall clock synchronization and error recovery.
 * 
 * Copyright (C) 2021 Ursescu Ionut 
 */

#include <iostream>
#include <chrono>
#include <vector>
#include <bitset>
#include <thread>

#include "phy.hh"
#include "lll.hh"

using namespace std::chrono;

/* Function that will sync to wall clock time, configured by LLConfig SYNC_MASK and SYNC_US */
static void sync(void)
{
	auto current = (duration_cast<microseconds>(
				system_clock::now().time_since_epoch())
				.count() %
			LLConfig::SYNC_MASK) /
		       LLConfig::SYNC_US;

	while ((duration_cast<microseconds>(
			system_clock::now().time_since_epoch())
			.count() %
		LLConfig::SYNC_MASK) /
		       LLConfig::SYNC_US ==
	       current) {
		; // Just wait here
	}
}

/* 
 * Prepare the pdu for phy, add bit stuffing for end sequence
 * Order: {Start sequence} | Seq | Type | CRC | Data | {End sequence}
 * Endianess: Little
 */
static void pdu_to_phy(ll_dev &dev, ll_pdu &pdu, std::vector<BIT> &frame_bits)
{
	using bitset_8 = std::bitset<8>;
	std::vector<BIT> message;
	uint8_t count = 0U;
	bitset_8 conv_bitset;
	uint32_t i;

	/* Add sequence number */
	conv_bitset = bitset_8(pdu.seq);
	for (i = 0U; i < conv_bitset.size(); i++) {
		message.emplace_back(conv_bitset[i]);
	}

	/* Add type */
	conv_bitset = bitset_8((uint8_t)pdu.type);
	for (i = 0U; i < conv_bitset.size(); i++) {
		message.emplace_back(conv_bitset[i]);
	}

	/* Add CRC */
	conv_bitset = bitset_8(pdu.crc);
	for (i = 0U; i < conv_bitset.size(); i++) {
		message.emplace_back(conv_bitset[i]);
	}

	/* Add data */
	for (i = 0U; i < pdu.data.size(); i++) {
		conv_bitset = bitset_8(pdu.data[i]);
		for (uint8_t j = 0U; j < conv_bitset.size(); j++) {
			message.emplace_back(conv_bitset[j]);
		}
	}

	/* Add start sequece */
	for (i = 0U; i < LLConfig::start_seq.size(); i++) {
		frame_bits.emplace_back(LLConfig::start_seq[i]);
	}

	/* Bit stuffing */
	for (auto bit : message) {
		if (bit == BIT_1) {
			count++;
		} else {
			count = 0U;
		}

		frame_bits.emplace_back(bit);

		if (count == 5U) {
			frame_bits.emplace_back(BIT_0);
			count = 0U;
		}
	}

	/* Add end sequence */
	for (i = 0U; i < LLConfig::end_seq.size(); i++) {
		frame_bits.emplace_back(LLConfig::end_seq[i]);
	}
}

/*
 * Prepare the phy data for ll pdu remove bit stuffing
 * Order: {Start sequence} | Seq | Type | CRC | Data | {End sequence}
 * Endianess: Big
 */
static bool phy_to_pdu(ll_dev &dev, ll_pdu &pdu, std::vector<BIT> frame_bits)
{
	using bitset_8 = std::bitset<8>;
	uint8_t count = 0U;
	uint32_t offset = 0U;
	bitset_8 conv_bitset;
	uint32_t data_size;
	uint8_t type;
	uint32_t i, byte;

	/* Sanity check */
	if (frame_bits.size() < LLConfig::MIN_PDU_LEN * BYTE_SIZE) {
		return false;
	}

	/* Remove start sequence */
	for (i = 0U; i < LLConfig::start_seq.size(); i++) {
		frame_bits.erase(frame_bits.begin());
	}

	/* Remove end sequence */
	for (i = 0U; i < LLConfig::end_seq.size(); i++) {
		frame_bits.pop_back();
	}
	/* Remove bit stuffing */
	for (auto bit = frame_bits.begin(); bit != frame_bits.end(); bit++) {
		if (*bit == BIT_1) {
			count++;
		} else {
			count = 0U;
		}

		if (count == 5U) {
			frame_bits.erase(bit + 1);
			count = 0U;
		}
	}

	/* Add sequence number */
	pdu.seq = 0U;
	for (i = 0U; i < BYTE_SIZE; i++) {
		pdu.seq |= (frame_bits[offset++] << i);
	}

	/* Add type */
	type = (pdu_type)0U;
	for (i = 0U; i < BYTE_SIZE; i++) {
		type |= (frame_bits[offset++] << i);
	}
	pdu.type = (pdu_type)type;

	/* Add crc */
	pdu.crc = 0U;
	for (i = 0U; i < BYTE_SIZE; i++) {
		pdu.crc |= (frame_bits[offset++] << i);
	}

	data_size = (frame_bits.size() - offset) / 8;

	/* Add data */
	pdu.data.clear();
	for (byte = 0U; byte < data_size; byte++) {
		/* Add byte */
		uint8_t data = 0U;

		for (i = 0U; i < BYTE_SIZE; i++) {
			data |= (frame_bits[offset++] << i);
		}
		pdu.data.emplace_back(data);
	}

	return true;
}

static bool recv_prv(ll_dev &dev, ll_pdu &pdu)
{
	std::vector<BIT> frame_bits;
	uint8_t count = 0U;
	bool ret = true;
	BIT recv_bit;

	for (auto bit : LLConfig::start_seq) {
		recv_bit = phy_recv(dev.phy_config);

		if (recv_bit != bit) {
			ret = false;
			break;
		}
		frame_bits.emplace_back(recv_bit);
	}

	if (ret) {
		ret = false;

		/* Got the sync, now retreive data */
		for (uint32_t i = 0; i < (LLConfig::MAX_FRAME_BIT_LEN); i++) {
			recv_bit = phy_recv(dev.phy_config);

			frame_bits.emplace_back(recv_bit);

			if (recv_bit == BIT_0) {
				count = 0U;
			}
			if (recv_bit == BIT_1) {
				count++;

				/* End sequence 01111110 */
				if (count == 6U) {
					frame_bits.emplace_back(BIT_0);
					ret = true;
					break;
				}
			}
		}
	}
	if (ret) {
		ret = phy_to_pdu(dev, pdu, frame_bits);
	}
	return ret;
}

static bool send_prv(ll_dev &dev, ll_pdu &pdu)
{
	std::vector<BIT> frame_bits;

	/* Prepare frame bits */
	pdu_to_phy(dev, pdu, frame_bits);

	/* Sync device */
	sync();

	/* Send payload */
	for (auto bit : frame_bits) {
		phy_send(dev.phy_config, bit);
	}

	return true;
}

/* RECV thread function */
static void recv_thread(ll_dev &dev, std::future<void> signal)
{
	while (signal.wait_for(std::chrono::milliseconds(0U)) == std::future_status::timeout) {
		ll_pdu pdu;

		/* Start by syncing to wall clock */
		sync();

		/* Recv pdu, push it to higher layer, use move semantic to pass the data */
		if (recv_prv(dev, pdu)) {
			dev.lll_rx_queue.push(std::move(pdu));
		}
	}
}

/* SEND thread function */
static void send_thread(ll_dev &dev, std::future<void> signal)
{
	ll_pdu pdu;

	dev.lll_tx_queue.wait_pop(pdu);
	while (signal.wait_for(std::chrono::milliseconds(0U)) == std::future_status::timeout) {
		send_prv(dev, pdu);

		dev.lll_tx_queue.wait_pop(pdu);
	}
}

void lll_init(ll_dev &dev)
{
	std::future<void> signal_rx = dev.lll_rx_thread_signal.get_future();
	std::future<void> signal_tx = dev.lll_tx_thread_signal.get_future();

	/* Start the threads */
	dev.lll_rx_thread =
		std::thread(recv_thread, std::ref(dev), std::move(signal_rx));

	dev.lll_tx_thread =
		std::thread(send_thread, std::ref(dev), std::move(signal_tx));
}

void lll_clear(ll_dev &dev)
{
	ll_pdu exit_pdu;

	dev.lll_rx_thread_signal.set_value();
	dev.lll_tx_thread_signal.set_value();

	/* Send thread is blocked until receiving a pdu, send dummy pdu*/
	lll_send(dev, exit_pdu);

	dev.lll_tx_thread.join();
	dev.lll_rx_thread.join();
}

bool lll_recv(ll_dev &dev, ll_pdu &pdu)
{
	ll_pdu local;
	bool ret = false;

	if (dev.lll_rx_queue.try_pop(local)) {
		pdu = local;
		ret = true;
	}

	return ret;
}

bool lll_send(ll_dev &dev, ll_pdu &pdu)
{
	bool ret = false;

	if (dev.lll_tx_queue.size() < LLConfig::LLL_TX_QUEUE_SIZE) {
		dev.lll_tx_queue.push(std::move(pdu));
		ret = true;
	}

	return ret;
}
