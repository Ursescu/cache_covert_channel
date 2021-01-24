CFLAGS = -std=c++14 -g -lpthread -O2
CC = g++

SOURCES = ll/lll.cc ll/ull.cc phy/phy.cc
OBJECTS = $(SOURCES:.cc=.o)

INCLUDES = -Ill -Iphy -Iutils
BUILDDIR = build

TARGETS = receiver sender

all: receiver sender

%.o: %.cc
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

receiver: %:%.o $(OBJECTS)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@

sender: %:%.o $(OBJECTS)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@

.PHONY:	clean

clean:
	rm -rf *.o $(TARGETS) $(OBJECTS)
