#include <iostream>
#include <bitset>
#include <chrono>
#include <future>
#include <thread>

#include "phy.hh"
#include "ll.hh"

using namespace std::chrono;

int main(int argc, char **argv)
{
	std::cout << "Sender\n";

	std::chrono::seconds timeout(1);

	ll_dev dev;

	ll_init(dev, CacheConfig::CHANNEL_1, CacheConfig::CHANNEL_0);

	std::thread output([&] {
		std::vector<uint8_t> recv;
		while (true) {
			if (ll_recv(dev, recv)) {
				for (auto byte : recv) {
					std::cout << byte << std::flush;
				}
			}
		}
	});

	while (true) {
		std::string to_send;
		std::vector<uint8_t> data_send;

		// Get command from user
		std::getline(std::cin, to_send);

		// std::cout << "Sending str " << to_send << "\n";
		for (auto chr : to_send) {
			data_send.emplace_back(static_cast<uint8_t>(chr));
		}
		data_send.emplace_back('\n');

		while (data_send.size() > 0) {
			std::vector<uint8_t> data;
			bool sent = false;
			for (auto byte = data_send.begin(); byte != data_send.end(); byte++) {
				data.emplace_back(*byte);
				data_send.erase(byte);
				byte--;

				if (data.size() >= LLConfig::MAX_SDU_LEN) {
					break;
				}
			}
			if (data.size() > 0) {
				do {
					// std::cout << "Sending data " << data.size() << "\n";
					if (!ll_send(dev, data)) {
						std::this_thread::sleep_for(std::chrono::milliseconds(50));
					} else {
						sent = true;
					}
				} while (!sent);
			}
		}
	}

	ll_clear(dev);

	output.join();

	return 0;
}