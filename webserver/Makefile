FILES=main.cpp webserver.cpp
OBJS=main.o webserver.o
CC=gcc
CFLAGS=-std=gnu++11 -c -Wall -Wextra -static

webserver: 
	g++ $(CFLAGS) $(FILES)
	g++ -o webserver $(OBJS)

clean:
	rm -rf $(OBJS)

distclean:
	rm -rf $(OBJS) webserver



