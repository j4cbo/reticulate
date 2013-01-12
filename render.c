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

#define MOVE_PATCHES	12
#define SPIN_PATCHES	12

#define PATCHES (MOVE_PATCHES + SPIN_PATCHES)

#define TAU (2 * M_PI)
#define INCR (TAU / SPIN_PATCHES)

static struct nurbs_patch *patches[PATCHES];

void render_init(void) {
	struct nurbs_line *circle_line = nurbs_load_line("data/circle.nub"),
	                  *square_line = nurbs_load_line("data/square.nub");
	assert(circle_line);
	assert(square_line);

	for (int i = 0; i < MOVE_PATCHES; i++)
		patches[i] = make_move(circle_line, move_path[i]);

	for (int i = 0; i < SPIN_PATCHES; i++) {
		float theta = INCR * i;
		float angles[] = {
			theta,
			fmod(theta + INCR*1/4, TAU),
			fmod(theta + INCR*3/4, TAU),
			fmod(theta + INCR, TAU),
		};

		patches[MOVE_PATCHES + i] = make_spin(square_line, angles);
	}
}

static int16_t clamp(float v) {
	return v >= 32767 ? 32767 : (v <= -32768 ? -32768 : v);
}

void render_point(struct etherdream_point *pt, float u, float redraw_count) {
	/* Figure out which patch we're in */
	float ipart, fpart = modff(u * PATCHES, &ipart);
	int patch = ipart;

	/* Evaluate the NURBS surface */
	struct xy xy = nurbs_evaluate(patches[patch],
	                              fmod(u * redraw_count, 1.0), fpart);

	/* Convert to DAC format */
	pt->x = clamp(xy.x * 10000);
	pt->y = clamp(xy.y * 10000);
	pt->r = 65535;
	pt->g = 65535;
	pt->b = 65535;
}
