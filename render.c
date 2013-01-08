#include <math.h>
#include <stdio.h>

#include "render.h"

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

struct nurbs_point circle0[] = CIRC(0.0, 0.0);
struct nurbs_point circle1[] = CIRC(0.0, 0.0);
struct nurbs_point circle2[] = CIRC(1.0, 0.0);
struct nurbs_point circle3[] = CIRC(3.0, 0.0);

float circle_knots[] = { 0, 0, 0, .25, .25, .5, .5, .75, .75, 1, 1, 1 };

struct nurbs_patch moving_circle = {
	9,
	{ circle0, circle1, circle2, circle3 },
	circle_knots
};

struct xy render_point(float u) {
	return nurbs_evaluate(&moving_circle, fmod(u * 10, 1.0), u);
}
