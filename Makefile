SRCS = main.c nurbs.c render.c ../j4cDAC/driver/libetherdream/etherdream.c

SDL_FLAGS = -framework SDL -framework Cocoa -framework OpenGL -I/Library/Frameworks/SDL.framework/Headers

all: talk

CFLAGS = -std=c99 -Wall -I../j4cDAC/driver/libetherdream -I../j4cDAC/common

reticulate: $(SRCS) SDLmain.m
	clang $(CFLAGS) $^ $(SDL_FLAGS) -o $@

talk: nurbs.c render.c ../j4cDAC/driver/libetherdream/etherdream.c tmain.c
	clang $(CFLAGS) $^ -o $@

clean:
	rm -f reticulate talk
