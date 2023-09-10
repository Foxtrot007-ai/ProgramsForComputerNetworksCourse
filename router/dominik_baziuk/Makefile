FILES=main.cpp router.cpp
OBJS=main.o router.o
CC=gcc
CFLAGS=-std=gnu++11 -c -Wall -Wextra -static

router: 
	g++ $(CFLAGS) $(FILES)
	g++ -o router $(OBJS)

clean:
	rm -rf $(OBJS)

distclean:
	rm -rf $(OBJS) router




