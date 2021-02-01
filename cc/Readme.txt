Cache covert channel simple protocol description
================================================

Ursescu Ionut

Physical layer
==============

Based on FLUSH+RELOAD on LLC cache. Finding the probing/eviction address sets
for sender and receiver. Flush the L1 cache on the sender and probe on the receiver.
When a cache hit ratio < 0.5 that means we have a `1` (we flushed most of the time)
otherwise, we have a `0` (no-one flushed the data from cache).

To make it work over the system, flushes need to be synchronized with probings.
Using the wall timer to sync the receiver with the sender (globally available information in a system on 
different VM or different processes).

Note:

By nature, the sender will not be able to receive anything because of the
busy loop flushing the address set. Same thing on the receiver. I avoided it by implementing it on multiple threads.

Data link layer
===============

LL is split into sub-layers. One is called Lower Link Layer (lll), which is responsible for maintaining
the Recv and Send threads. Also, it does the wall clock synchronization (described in the Physical Layer part).
The other one is called Upper Link Layer and is responsible for creating the PDU type from SDU (user data) and
the sliding window protocol (ACK, Timeout, Retry, CRC).

User Interface
==============

Demo application just to prove the stack functionality. The receiver is a reverse shell and is sending its output via
the Link layer to the other process. Also, the input is forwarded from the Sender via the Link layer to the reverse shell.

NOTES
=====

- Limited by the single-channel reading. Aligned at the 10ms mark on the wall clock, I have a theoretical speed of 35 bytes/s (Which is pretty small).
+ Can be implemented using multiple channels / Use prime and probe mechanism.

- Sliding window protocol has some flaws, sometimes the CRC is valid, but the PDU is wrong, this can cause Seg Faults.
+ Using 8 bits CRC, maybe by switching to something large the chance of false-positive will be lower. This will cause a slower transmission (see the first point).

+ Can be tuned to send faster. Need to find the sweet spot for configurations. (Windows size, Max PDU size, Retry timeout, etc).