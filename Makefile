SRCS = main.c render.c

SDL_FLAGS = -framework SDL -framework Cocoa -framework OpenGL -I/Library/Frameworks/SDL.framework/Headers

reticulate: $(SRCS) SDLmain.m
	gcc $(SRCS) SDLmain.m -Wall $(SDL_FLAGS) -o $@

clean:
	rm -f reticulate
