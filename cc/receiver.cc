#include <iostream>
#include <bitset>
#include <chrono>
#include <thread>

#include "ll.hh"
#include "phy.hh"


using namespace std::chrono;

// Entry point - test
int main(int argc, char **argv)
{
	std::cout << "Receiver\n";
	
	ll_dev dev;

	init_ll_dev(dev, CacheConfig::CHANNEL_1, CacheConfig::CHANNEL_0);

	lll_run(dev);

	ll_pdu send_pdu;
	ll_pdu recv_pdu;

	uint8_t count = 1;

	while(true) {
		if(lll_recv(dev, recv_pdu)) {
			std::cout << "Received pdu \n";
			std::cout << "Seq: " << static_cast<unsigned>(recv_pdu.seq) << "\n";
			std::cout << "Type: " << static_cast<unsigned>(recv_pdu.type) << "\n";
			std::cout << "Crc: " << static_cast<unsigned>(recv_pdu.crc) << "\n";
			std::cout << "Data: ";
			for (auto byte : recv_pdu.data) {
				std::cout << " " << static_cast<unsigned>(byte);
			}
			std::cout << "\n";
		}
		
		// std::this_thread::sleep_for(milliseconds(2000));

		// ll_pdu send_pdu;
		// send_pdu.seq = count;
		// send_pdu.crc = 30U;
		// send_pdu.type = PDU_DATA_REQ;
		// send_pdu.data.clear();
		// send_pdu.data.emplace_back(1U);
		// send_pdu.data.emplace_back(0b01111110);
		// lll_send(dev, send_pdu);

		// std::this_thread::sleep_for(milliseconds(2000));
		// count++;

		// if (count == 0xff) {
		// 	count = 1U;
		// }
	}


	lll_wait_exit(dev);

	return 0;
}
