SRCS = main.c

CC = gcc

CFLAGS += -std=c99 -g -Wall
CFLAGS += -framework SDL -framework Cocoa -framework OpenGL
CFLAGS += -I/Library/Frameworks/SDL.framework/Headers
CFLAGS += -I../../j4cDAC/common

virt: $(SRCS) SDLmain.m
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f virt
