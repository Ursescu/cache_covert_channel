#include <iostream>
#include <bitset>

#include "covert.hh"


void send_bit(SenderConfig &config, BIT bit) {

    if (bit == 0)
        send_zero(config);
    else
        send_one(config);
}

// Entry point - test
int main(int argc, char **argv) {

    std::cout << "Sender" << std::endl;
    // Configuration

    SenderConfig config;

    std::cout << "Period " << std::dec << config.period << std::endl;

    build_eviction_list(config);
    uint32_t counter = 0;

    std::cout << "Eviction list addresses:" << std::endl;

    for (auto it = config.eviction_list.begin(); it != config.eviction_list.end(); it++, counter++) {

        std::cout << counter << ": 0x" << std::hex << *it << std::dec<< std::endl;
    }

    // Listen

    std::cout << "Send now. Limitation without 0 at the end of data!!!!" << std::endl;

    std::string reading;
    while(true) {
        
        std::vector<BIT> data;
        bool validInput = true;


        std::cout << "Send something (0|1):";
        std::cin >> reading;

        for (char const &c: reading) {
            if (c == '0')
                data.emplace_back(0U);
            else if (c == '1')
                data.emplace_back(1U);
            else {
                validInput = false;
                std::cout << "Invalid input, only 0 or 1" << std::endl;
            }
        }

        if (!validInput)
            continue;

        std::cout << "Sending: ";
        for (auto it = data.begin(); it != data.end(); it++) {
            
            std::cout << std::dec << (uint32_t)(*it) << " ";
        }
        std::cout << std::endl;

        for(uint32_t i = 0; i < 100; i++) {

            for (auto it = ProtocolConfig::startSequence.begin(); it != ProtocolConfig::startSequence.end(); it++) {
                send_bit(config, *it);
            }

            for (auto it = data.begin(); it != data.end(); it++) {
                send_bit(config, *it);
            }

            for (auto it = ProtocolConfig::endSequence.begin(); it != ProtocolConfig::endSequence.end(); it++) {
                send_bit(config, *it);
            }
        }

        std::cout << "Sender finished." << std::endl;
    }

    return 0;
}