#include <iostream>
#include <chrono>
#include <vector>
#include <bitset>
#include <thread>

#include "phy.hh"
#include "ll.hh"
#include "ull.hh"

void ll_init(ll_dev &dev, uint32_t send_channel, uint32_t recv_channel)
{
	/* Set the seq number start */
	dev.curr_seq = 0U;

	/* Init phy device */
	init_phy_dev(dev.phy_config, send_channel, recv_channel);

	ull_start(dev);
}

void ll_clear(ll_dev &dev)
{
	/* Wait here until everything is stopped */
	ull_clear(dev);

    /* Now reset everyting in dev */
    dev.curr_seq = 0U;
}

bool ll_send(ll_dev &dev, std::vector<uint8_t> &data)
{
	bool ret = false;

	/* Check if there is space */
	if (dev.ull_tx_queue.size() < LLConfig::ULL_TX_QUEUE_SIZE) {
		/* Prepare pachet for ull */
		ll_pdu pdu;

		/* Invoke copy */
		pdu.data = data;

        dev.ull_tx_queue.push(std::move(pdu));

		ret = true;
	}

	return ret;
}

bool ll_recv(ll_dev &dev, std::vector<uint8_t> &data)
{
	bool ret = false;
    ll_pdu pdu;

	if (dev.ull_rx_queue.try_pop(pdu)) {
        
		/* Invoke copy */
        data = pdu.data;

        ret = true;
    }

    return ret;
}

uint8_t ll_pdu_crc(ll_pdu &pdu)
{
	uint8_t *buffer;
	uint32_t size = 0U;
	uint8_t crc = 0U;

	size += pdu.data.size();

	/* CRC + SEQ + Type */
	size += LLConfig::MIN_PDU_LEN;

	buffer = new uint8_t[size];

	/* Populate buffer */
	buffer[0U] = pdu.seq;
	buffer[2U] = static_cast<uint8_t>(pdu.type);
	buffer[1U] = 0U; // Ignore CRC value

	for (uint32_t i = 0; i < pdu.data.size(); i++) {
		buffer[i + 3] = pdu.data[i];
	}

	/* Calculate CRC */
	crc = CRC::Calculate(buffer, size, CRC::CRC_8());

	delete buffer;

	return crc;
}

/* Automate the crc calculation */
ll_pdu::ll_pdu(std::vector<uint8_t> &data, uint8_t seq) : data(data), seq(seq)
{
	this->crc = 0U;
};

ll_pdu::ll_pdu(std::vector<uint8_t> &data, uint8_t seq, uint8_t crc)
	: data(data), seq(seq), crc(crc){};

std::vector<BIT> LLConfig::start_seq = { 0, 1, 1, 1, 1, 1, 1, 0 };
std::vector<BIT> LLConfig::end_seq = { 0, 1, 1, 1, 1, 1, 1, 0 };
