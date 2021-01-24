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

	ll_init(dev, CacheConfig::CHANNEL_1, CacheConfig::CHANNEL_0);

	std::this_thread::sleep_for(std::chrono::seconds(3));

	ll_clear(dev);

	return 0;
}
