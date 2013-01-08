#include <math.h>
#include <stdio.h>

#include "render.h"

#define CIRC(xoff, yoff) ((struct nurbs_point[]){ \
	{ 1 + xoff, 0 + yoff, 1 }, \
	{ 1 + xoff, 1 + yoff, M_SQRT1_2 }, \
	{ 0 + xoff, 1 + yoff, 1 }, \
	{ -1 + xoff, 1 + yoff, M_SQRT1_2 }, \
	{ -1 + xoff, 0 + yoff, 1 }, \
	{ -1 + xoff, -1 + yoff, M_SQRT1_2 }, \
	{ 0 + xoff, -1 + yoff, 1 }, \
	{ 1 + xoff, -1 + yoff, M_SQRT1_2 }, \
	{ 1 + xoff, 0 + yoff, 1 }, \
})

float circle_knots[] = { 0, 0, 0, .25, .25, .5, .5, .75, .75, 1, 1, 1 };

const struct nurbs_patch moving_circles[] = {
	{ 9, { CIRC(-2.0,  2.0), CIRC(-2.0,  2.0), CIRC(-1.5,  2.0), CIRC(-1.0,  2.0) }, circle_knots },
	{ 9, { CIRC(-1.0,  2.0), CIRC(-0.5,  2.0), CIRC( 0.5,  2.0), CIRC( 1.0,  2.0) }, circle_knots },
	{ 9, { CIRC( 1.0,  2.0), CIRC( 1.5,  2.0), CIRC( 2.0,  2.0), CIRC( 2.0,  2.0) }, circle_knots },

	{ 9, { CIRC( 2.0,  2.0), CIRC( 2.0,  2.0), CIRC( 2.0,  1.5), CIRC( 2.0,  1.0) }, circle_knots },
	{ 9, { CIRC( 2.0,  1.0), CIRC( 2.0,  0.5), CIRC( 2.0, -0.5), CIRC( 2.0, -1.0) }, circle_knots },
	{ 9, { CIRC( 2.0, -1.0), CIRC( 2.0, -1.5), CIRC( 2.0, -2.0), CIRC( 2.0, -2.0) }, circle_knots }, 

	{ 9, { CIRC( 2.0, -2.0), CIRC( 2.0, -2.0), CIRC( 1.5, -2.0), CIRC( 1.0, -2.0) }, circle_knots },
	{ 9, { CIRC( 1.0, -2.0), CIRC( 0.5, -2.0), CIRC(-0.5, -2.0), CIRC(-1.0, -2.0) }, circle_knots },
	{ 9, { CIRC(-1.0, -2.0), CIRC(-1.5, -2.0), CIRC(-2.0, -2.0), CIRC(-2.0, -2.0) }, circle_knots },

	{ 9, { CIRC(-2.0, -2.0), CIRC(-2.0, -2.0), CIRC(-2.0, -1.5), CIRC(-2.0, -1.0) }, circle_knots },
	{ 9, { CIRC(-2.0, -1.0), CIRC(-2.0, -0.5), CIRC(-2.0,  0.5), CIRC(-2.0,  1.0) }, circle_knots },
	{ 9, { CIRC(-2.0,  1.0), CIRC(-2.0,  1.5), CIRC(-2.0,  2.0), CIRC(-2.0,  2.0) }, circle_knots },
};

struct xy render_point(float u, float per_circle) {
	double ipart;
	u = modf(u, &ipart);
	u *= 12.0;
	float fpart = modf(u, &ipart);
	int i = ipart;
	return nurbs_evaluate(&moving_circles[i], fmod(u * per_circle / 12.0, 1.0), fpart);
}
