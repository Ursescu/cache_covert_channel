#include <iostream>
#include <bitset>
#include <chrono>
#include <thread>

#include "phy.hh"
#include "ll.hh"

using namespace std::chrono;

int main(int argc, char **argv)
{
	std::cout << "Sender\n";

	ll_dev dev;

	ll_init(dev, CacheConfig::CHANNEL_1, CacheConfig::CHANNEL_0);

	std::this_thread::sleep_for(std::chrono::seconds(3));

	ll_clear(dev);

	return 0;
}