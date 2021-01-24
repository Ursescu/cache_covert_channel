#include <iostream>
#include <chrono>
#include <vector>
#include <bitset>
#include <thread>
#include <future>

#include "phy.hh"
#include "ll.hh"
#include "lll.hh"

/* ULL thread function */
static void ull_thread(ll_dev &dev, std::future<void>signal) {

    /* Init segment and window */


	while(signal.wait_for(std::chrono::milliseconds(0U)) == std::future_status::timeout) {
        std::cout << "ULL task\n";
 
       
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}    

}

void ull_start(ll_dev &dev) {
    std::future<void> signal = dev.ull_thread_signal.get_future();

    dev.ull_thread = std::thread(ull_thread, std::ref(dev), std::move(signal));

    /* Also start the lower link layer threads */
    lll_init(dev);
}

/* Stop ull */
extern void ull_clear(ll_dev &dev) {

    /* Stop thread */
    dev.ull_thread_signal.set_value();

    dev.ull_thread.join();

    lll_clear(dev);
}
