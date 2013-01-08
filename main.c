#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include <math.h>

#include "render.h"

#define FPS 30
#define PIXELS_PER_UNIT	120

#define WINDOW_SIZE 800

SDL_Surface * screen;

void init(int width) {
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		exit(-1);
	}

	screen = SDL_SetVideoMode(width, width, 24,
		SDL_OPENGL | SDL_GL_DOUBLEBUFFER);

	glEnable(GL_TEXTURE_2D);
	glViewport(0, 0, width, width);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0f, width, width, 0.0f, -1.0f, 1.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void renderPoints() {
	int i;

	/* Draw a grid */
	glBegin(GL_LINES);
	glColor3f(0.3, 0.3, 0.3);
	for (i = -5; i < 5; i++) {
		glVertex2i(i * PIXELS_PER_UNIT, -1000);
		glVertex2i(i * PIXELS_PER_UNIT, 1000);
		glVertex2i(-1000, i * PIXELS_PER_UNIT);
		glVertex2i(1000, i * PIXELS_PER_UNIT);
	}
	glEnd();	

	/* Draw some output */
	glBegin(GL_LINE_STRIP);
	for (i = 0; i < 360000; i++) {
		float u = i / 360000.0;
		struct xy xy = render_point(u, 25);
		glColor3f(u, 0, 1.0 - u);
		glVertex2i(xy.x * PIXELS_PER_UNIT, xy.y * PIXELS_PER_UNIT);
	}
	glEnd();
}

void run() {
	/* Handle SDL events */
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		default:
			break;
		}
	}

	glClear(GL_COLOR_BUFFER_BIT);
	renderPoints();
	SDL_GL_SwapBuffers();
	SDL_Delay(1000 / FPS);
}

int main(int argc, char **argv) {
	init(WINDOW_SIZE);

	glTranslatef(WINDOW_SIZE / 2, WINDOW_SIZE / 2, 0);

	while (!SDL_QuitRequested())
		run();

	return 0;
}
