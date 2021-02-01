#include <iostream>
#include <bitset>
#include <chrono>
#include <thread>
#include <string>

#include "ll.hh"
#include "phy.hh"

#include "subprocess.hpp"

static void print_arr(std::vector<uint8_t> arr) {
	std::cout << "[";
	for (auto elem = arr.begin(); elem != arr.end(); elem++) {

		std::cout << static_cast<unsigned>(*elem);

		if (elem + 1 != arr.end()) {
			std::cout << ", ";
		}
	}
	std::cout << "]";
}

using namespace std::chrono;

// Entry point - test
int main(int argc, char **argv)
{
	std::cout << "Receiver\n";

	ll_dev dev;

	subprocess::popen cmd("bash", {});

	ll_init(dev, CacheConfig::CHANNEL_0, CacheConfig::CHANNEL_1);

	std::vector<uint8_t> data_send;

	while (true) {
		std::vector<uint8_t> recv;
		std::vector<uint8_t> data;
		bool sent = false;

		while (cmd.stdout().rdbuf()->in_avail() != 0) {
			char t;

			cmd.stdout().get(t);
			data_send.emplace_back(t);
		}
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
				sent = ll_send(dev, data);
			} while (!sent);
		}

		while (ll_recv(dev, recv)) {
			for (auto byte : recv) {
				std::cout << byte << std::flush;
				cmd.stdin() << byte;
			}
		}
	}

	ll_clear(dev);

	return 0;
}
