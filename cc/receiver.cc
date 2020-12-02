#include <iostream>
#include <bitset>

#include "covert.hh"

// Entry point - test
int main(int argc, char **argv)
{
	// Configuration
	ReceiverConfig config;

	std::cout << "Period " << std::dec << config.period << std::endl;

	build_probe_list(config);
	uint32_t counter = 0;

	std::cout << "Probe list addresses:" << std::endl;

	for (auto it = config.probe_list.begin(); it != config.probe_list.end();
	     it++, counter++) {
		std::cout << counter << ": 0x" << std::hex << *it << std::dec
			  << std::endl;
	}

	// Listen
	std::cout << "Receiver now listening." << std::endl;
	bool listening = true;

	uint8_t bit;
	uint32_t numberOfSequences = 0U;

	while (listening) {
		bool foundSequence = true;

		for (auto it = ProtocolConfig::startSequence.begin();
		     it != ProtocolConfig::startSequence.end(); it++) {
			bit = listen_for_bit(config);

			if (bit != *it) {
				foundSequence = false;
			}
		}

		if (!foundSequence)
			continue; // Try again

		numberOfSequences += 1;

		std::cout << "Synced with sender: " << numberOfSequences
			  << std::endl;

		std::vector<uint32_t> data;
		uint32_t consecZeros = 0U;

		for (uint32_t i = 0; i < (128 * 8); i++) {
			bit = listen_for_bit(config);

			if (bit == 1) {
				data.emplace_back(1U);
				consecZeros = 0U;
			}
			if (bit == 0) {
				consecZeros++;

				/* Final sequence */
				data.emplace_back(0U);

				/* Hardcoded end sequence 8 * 0 */
				if (consecZeros == 8U) {
					std::cout << "End sequence detected"
						  << std::endl;

					for (uint32_t i = 0; i < 8; i++)
						data.pop_back();
					break;
				}
			}
		}

		for (auto it = data.begin(); it != data.end(); it++) {
			std::cout << *it;
		}

		std::cout << std::endl;
	}
	return 0;
}
