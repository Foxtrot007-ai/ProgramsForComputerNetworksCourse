FILES=main.cpp send_packet.cpp receive_packets.cpp
OBJS=main.o receive_packets.o send_packet.o
CC=gcc
CFLAGS=-std=gnu++17 -c -Wall -Wextra

traceroute: 
	g++ $(CFLAGS) $(FILES)
	g++ -o traceroute $(OBJS)

clean:
	rm -rf $(OBJS)

distclean:
	rm -rf $(OBJS) traceroute




