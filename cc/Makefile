CFLAGS=-std=c++11 -g
CC=g++

TARGETS=receiver sender

all: receiver sender

covert.o: %.o: %.cc %.hh
	$(CC) $(CFLAGS) -c $<

%.o: %.cc
	$(CC) $(CFLAGS)  -c $< 

receiver: %:%.o covert.o
	$(CC) $(CFLAGS) $^ -o $@

sender: %:%.o covert.o
	$(CC) $(CFLAGS) $^ -o $@

.PHONY:	clean

clean:
	rm *.o $(TARGETS) 
