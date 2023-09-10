FILES=main.cpp transport.cpp
OBJS=main.o transport.o
CC=gcc
CFLAGS=-std=gnu++11 -c -Wall -Wextra -static

transport: 
	g++ $(CFLAGS) $(FILES)
	g++ -o transport $(OBJS)

clean:
	rm -rf $(OBJS)

distclean:
	rm -rf $(OBJS) transport




