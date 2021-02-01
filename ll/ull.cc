#include <iostream>
#include <chrono>
#include <vector>
#include <bitset>
#include <thread>
#include <future>

#include "phy.hh"
#include "ll.hh"
#include "lll.hh"
#include "ull.hh"

struct ull_tx_window_elem {
	bool acked;
	ll_pdu pdu;

	// Time ref to implement retry
	std::chrono::time_point<std::chrono::system_clock> timestamp;

	ull_tx_window_elem() = default;
	ull_tx_window_elem(ll_pdu &pdu) : pdu(pdu){};
};

struct ull_rx_window_elem {
	uint8_t seq;
	bool recved;
	ll_pdu pdu;

	ull_rx_window_elem() = default;
};

static uint8_t get_next_rx_seq(ll_dev &dev)
{
	return (dev.rx_seq + 1) % LLConfig::SEGMENT_SIZE;
}

static uint8_t get_next_tx_seq(ll_dev &dev)
{
	return (dev.tx_seq + 1) % LLConfig::SEGMENT_SIZE;
}

static void send_ack(ll_dev &dev, ll_pdu &pdu)
{
	ll_pdu ack;
	bool sent = false;

	ack.type = PDU_ACK;
	ack.seq = pdu.seq;
	ack.data.emplace_back(0);
	ack.crc = ll_pdu_crc(ack);

	while (!sent) {
		sent = lll_send(dev, ack);
	}
}

static std::chrono::time_point<std::chrono::system_clock>
get_current_micros(void)
{
	return std::chrono::system_clock::now();
}

static int64_t
time_diff(std::chrono::time_point<std::chrono::system_clock> current)
{
	return std::chrono::duration_cast<std::chrono::microseconds>(
								 std::chrono::system_clock::now() - current)
					.count();
}

/* ULL thread function */
static void ull_thread(ll_dev &dev, std::future<void> signal)
{
	/* Init tx and rx window */
	std::vector<ull_tx_window_elem> tx_window;

	std::vector<ull_rx_window_elem> rx_window;

	for (uint8_t i = 0U; i < LLConfig::WINDOW_SIZE; i++) {
		ull_rx_window_elem window_elem;

		window_elem.seq = get_next_rx_seq(dev);
		window_elem.recved = false;

		rx_window.emplace_back(window_elem);

		dev.rx_seq = get_next_rx_seq(dev);
	}

	while (signal.wait_for(std::chrono::milliseconds(0U)) ==
								 std::future_status::timeout ||
				 (tx_window.size() != 0)) {
		ll_pdu recv_pdu;
		ll_pdu send_pdu;

		// Received something
		if (lll_recv(dev, recv_pdu)) {

			// Check for CRC
			if (ll_pdu_crc(recv_pdu) == recv_pdu.crc) {
				switch (recv_pdu.type) {
				case PDU_DATA:

					// Send ack for it
					send_ack(dev, recv_pdu);

					// Check if we waiting on this packet
					for (auto elem = rx_window.begin(); elem != rx_window.end(); elem++) {
						if (elem->seq == recv_pdu.seq) {
							if (!elem->recved) {
								elem->pdu = recv_pdu;
								elem->recved = true;
							}
						}
					}
					/* code */
					break;
				case PDU_ACK:

					for (auto elem = tx_window.begin(); elem != tx_window.end(); elem++) {
						if (elem->pdu.seq == recv_pdu.seq && !elem->acked) {
							elem->acked = true;
						}
					}
					break;
				default:
					std::cout << "Unknown type for seq "
										<< static_cast<unsigned>(recv_pdu.seq) << ", drop it\n";
					break;
				}
			}
		}

		// Delete all the acked from start tx_window
		while (tx_window.size() > 0U && tx_window.begin()->acked) {
			tx_window.erase(tx_window.begin());
		}

		// Delete all the acked from start from rx_window
		while (rx_window.size() > 0U && rx_window.begin()->recved) {
			ull_rx_window_elem window_elem;

			//Push recived data to higer layer
			dev.ull_rx_queue.push(std::move(rx_window.begin()->pdu));

			// Delete it from window
			rx_window.erase(rx_window.begin());

			// Add new to rx_window
			window_elem.seq = get_next_rx_seq(dev);
			window_elem.recved = false;

			rx_window.emplace_back(window_elem);

			dev.rx_seq = get_next_rx_seq(dev);
		}

		// Add new tx packet if there is space
		while (tx_window.size() < LLConfig::WINDOW_SIZE &&
					 dev.ull_tx_queue.try_pop(send_pdu)) {
			ll_pdu local_pdu;
			ull_tx_window_elem window_elem;
			bool sent = false;

			// Generate local pdu
			local_pdu = send_pdu;
			local_pdu.type = PDU_DATA;
			local_pdu.seq = get_next_tx_seq(dev);
			local_pdu.crc = ll_pdu_crc(local_pdu);

			// Update window elem
			window_elem.pdu = local_pdu;
			window_elem.acked = false;
			window_elem.timestamp = get_current_micros();

			// Add to window
			tx_window.emplace_back(window_elem);

			dev.tx_seq = get_next_tx_seq(dev);

			// Send the frame
			while (!sent) {
				sent = lll_send(dev, local_pdu);
			}
		}

		for (auto elem = tx_window.begin(); elem != tx_window.end(); elem++) {
			if (!elem->acked &&
					(time_diff(elem->timestamp) > LLConfig::RETRY_TIMEOUT_US)) {
				ll_pdu pdu_copy = elem->pdu;
				bool sent = false;

				// Should retry send
				while (!sent) {
					sent = lll_send(dev, pdu_copy);
				}

				// Update timestamp
				elem->timestamp = get_current_micros();
			}
		}
	}
}

void ull_init(ll_dev &dev)
{
	std::future<void> signal = dev.ull_thread_signal.get_future();

	dev.ull_thread = std::thread(ull_thread, std::ref(dev), std::move(signal));

	/* Also start the lower link layer threads */
	lll_init(dev);
}

/* Stop ull */
extern void ull_clear(ll_dev &dev)
{
	/* Stop thread */
	dev.ull_thread_signal.set_value();

	dev.ull_thread.join();

	lll_clear(dev);
}
