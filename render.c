#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "render.h"

struct nurbs_patch *make_move(const struct nurbs_line *line,
                              struct xy path[NURBS_T_POINTS]) {

	size_t size = sizeof (struct nurbs_patch)
	            + sizeof (struct nurbs_point) * line->points * 4;

	struct nurbs_patch *out = malloc(size);
	assert(out);

	out->points = line->points;
	out->xy_knots = line->knots;

	for (int i = 0; i < line->points; i++) {
		for (int j = 0; j < 4; j++) {
			out->t[i][j].x = line->t[i].x + path[j].x;
			out->t[i][j].y = line->t[i].y + path[j].y;
			out->t[i][j].weight = line->t[i].weight;
		}
	}

	return out;
}

struct nurbs_patch *make_spin(const struct nurbs_line *line,
                              const float angles[4]) {

	size_t size = sizeof (struct nurbs_patch)
	            + sizeof (struct nurbs_point) * line->points * 4;

	struct nurbs_patch *out = malloc(size);
	assert(out);

	out->points = line->points;
	out->xy_knots = line->knots;

	for (int i = 0; i < line->points; i++) {
		for (int j = 0; j < 4; j++) {
			float tsin = sinf(angles[j]);
			float tcos = cosf(angles[j]);
			float x = line->t[i].x, y = line->t[i].y;

			out->t[i][j].x = x * tcos - y * tsin;
			out->t[i][j].y = x * tsin + y * tcos;
			out->t[i][j].weight = line->t[i].weight;
		}
	}

	return out;
}

struct xy move_path[][4] = {
	{ { -2.0,  2.0 }, { -2.0,  2.0 }, { -1.5,  2.0 }, { -1.0,  2.0 } },
	{ { -1.0,  2.0 }, { -0.5,  2.0 }, {  0.5,  2.0 }, {  1.0,  2.0 } },
	{ {  1.0,  2.0 }, {  1.5,  2.0 }, {  2.0,  2.0 }, {  2.0,  2.0 } },

	{ {  2.0,  2.0 }, {  2.0,  2.0 }, {  2.0,  1.5 }, {  2.0,  1.0 } },
	{ {  2.0,  1.0 }, {  2.0,  0.5 }, {  2.0, -0.5 }, {  2.0, -1.0 } },
	{ {  2.0, -1.0 }, {  2.0, -1.5 }, {  2.0, -2.0 }, {  2.0, -2.0 } },

	{ {  2.0, -2.0 }, {  2.0, -2.0 }, {  1.5, -2.0 }, {  1.0, -2.0 } },
	{ {  1.0, -2.0 }, {  0.5, -2.0 }, { -0.5, -2.0 }, { -1.0, -2.0 } },
	{ { -1.0, -2.0 }, { -1.5, -2.0 }, { -2.0, -2.0 }, { -2.0, -2.0 } },

	{ { -2.0, -2.0 }, { -2.0, -2.0 }, { -2.0, -1.5 }, { -2.0, -1.0 } },
	{ { -2.0, -1.0 }, { -2.0, -0.5 }, { -2.0,  0.5 }, { -2.0,  1.0 } },
	{ { -2.0,  1.0 }, { -2.0,  1.5 }, { -2.0,  2.0 }, { -2.0,  2.0 } },
};

#define PATCHES 24

struct nurbs_patch *moving_circles[PATCHES];

#define TAU (2 * M_PI)

#define INCR (TAU / (float)PATCHES / 2)

void render_init(void) {
	struct nurbs_line *line = nurbs_load_line("data/square.nub");
	assert(line);

	for (int i = 0; i < PATCHES; i++) {
		if (i < 12) {
			moving_circles[i] = make_move(line, move_path[i]);
		} else {
			float theta = INCR * (int)i - 12;
			float angles[] = {
				theta,
				fmod(theta + (1*INCR/4.0), TAU),
				fmod(theta + (3*INCR/4.0), TAU),
				fmod(theta + INCR, TAU),
			};
			moving_circles[i] = make_spin(line, angles);
		}
	}
}

struct xy render_point(float u, float per_circle) {
	double ipart;
	u = modf(u, &ipart);
	u *= (float)PATCHES;
	float fpart = modf(u, &ipart);
	int i = ipart;
	return nurbs_evaluate(moving_circles[i], fmod(u * per_circle / (float)PATCHES, 1.0), fpart);
}
