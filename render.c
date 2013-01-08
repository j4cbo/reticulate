#include <math.h>
#include <stdio.h>

#include "render.h"

struct point {
	float x;
	float y;
	float weight;
};

struct patch {
	int points;
	struct point *t[4];
	float *knots;
};

#define CIRC(xoff, yoff) { \
	{ 1 + xoff, 0 + yoff, 1 }, \
	{ 1 + xoff, 1 + yoff, M_SQRT1_2 }, \
	{ 0 + xoff, 1 + yoff, 1 }, \
	{ -1 + xoff, 1 + yoff, M_SQRT1_2 }, \
	{ -1 + xoff, 0 + yoff, 1 }, \
	{ -1 + xoff, -1 + yoff, M_SQRT1_2 }, \
	{ 0 + xoff, -1 + yoff, 1 }, \
	{ 1 + xoff, -1 + yoff, M_SQRT1_2 }, \
	{ 1 + xoff, 0 + yoff, 1 }, \
}

struct point circle0[] = CIRC(0.0, 0.0);
struct point circle1[] = CIRC(0.0, 0.0);
struct point circle2[] = CIRC(1.0, 0.0);
struct point circle3[] = CIRC(3.0, 0.0);

float circle_knots[] = { 0, 0, 0, .25, .25, .5, .5, .75, .75, 1, 1, 1 };

struct patch moving_circle = {
	9,
	{ circle0, circle1, circle2, circle3 },
	circle_knots
};

static float f(const float *knots, int i, int n, float u) {
	return (u - knots[i]) / (knots[i + n] - knots[i]);
}

static float g(const float *knots, int i, int n, float u) {
	return (knots[i + n] - u) / (knots[i + n] - knots[i]);
}

static float N1(const float *knots, int i, float u) {
	float out = 0;
	if (knots[i] <= u && u < knots[i + 1])
		out = f(knots, i, 1, u);
	if (knots[i + 1] <= u && u < knots[i + 2])
		out += g(knots, i + 1, 1, u);
	return out;
}

static float N2(const float *knots, int i, float u) {
	float out = 0;
	if (knots[i] <= u && u < knots[i + 2])
		out = f(knots, i, 2, u) * N1(knots, i, u);
	if (knots[i + 1] <= u && u < knots[i + 3])
		out += g(knots, i + 1, 2, u) * N1(knots, i + 1, u);
	return out;
}

static float t_knots[7] = { -1.5, -1.5, -.5, .5, 1.5, 2.5, 2.5 };

static struct xy render(struct patch *p, float u, float v) {
	int i, j;
	struct xy out = { 0, 0 };
	float div = 0;

	for (i = 0; i < p->points; i++) {
		for (j = 0; j < 4; j++) {
			float r = p->t[j][i].weight
			        * N2(p->knots, i, u)
			        * N2(t_knots, j, v);

			div += r;
			out.x += r * p->t[j][i].x;
			out.y += r * p->t[j][i].y;
		}
	}

	out.x /= div;
	out.y /= div;

	return out;
}

struct xy render_point(float u) {
	return render(&moving_circle, fmod(u * 10, 1.0), u);
}
