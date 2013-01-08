SRCS = main.c nurbs.c render.c ../j4cDAC/driver/libetherdream/etherdream.c

SDL_FLAGS = -framework SDL -framework Cocoa -framework OpenGL -I/Library/Frameworks/SDL.framework/Headers

all: reticulate talk

reticulate: $(SRCS) SDLmain.m
	gcc -std=c99 $(SRCS) SDLmain.m -Wall $(SDL_FLAGS) -I../j4cDAC/driver/libetherdream -I../j4cDAC/common -o $@

talk: nurbs.c render.c ../j4cDAC/driver/libetherdream/etherdream.c tmain.c
	gcc -std=c99 $^ -Wall -I../j4cDAC/driver/libetherdream -I../j4cDAC/common -o $@

clean:
	rm -f reticulate
