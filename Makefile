CC := gcc
CFLAGS := -Wall -O3 -std=c99 -I include

DEPS = \
	src/main.c   \
	src/socks4.c \
	src/tcp.c
 

socks: $(DEPS)
	$(CC) -o $@ $(CFLAGS) $^

debug: $(DEPS)
	$(CC) -o $@ $(CFLAGS) $^ -g

clean :
	rm -f socks
