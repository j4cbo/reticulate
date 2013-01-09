#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "render.h"

const float nurbs_t_knots[] = { 0, 0, 0, 0.5, 1, 1, 1 };

struct nurbs_patch *make_move(int npts,
                              const struct nurbs_point *pts,
                              const float *knots,
                              struct xy path[NURBS_T_POINTS]) {

	size_t size = sizeof (struct nurbs_patch)
	            + sizeof (struct nurbs_point) * npts * 4;

	struct nurbs_patch *out = malloc(size);
	assert(out);

	out->points = npts;
	out->xy_knots = knots;
	out->t_knots = nurbs_t_knots;

	for (int i = 0; i < npts; i++) {
		for (int j = 0; j < 4; j++) {
			out->t[i][j].x = pts[i].x + path[j].x;
			out->t[i][j].y = pts[i].y + path[j].y;
			out->t[i][j].weight = pts[i].weight;
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

struct nurbs_patch *moving_circles[12];

void render_init(void) {
	struct nurbs_line *line = nurbs_load_line("data/circle.nub");
	assert(line);

	for (int i = 0; i < 12; i++)
		moving_circles[i] = make_move(line->points, line->t, line->knots, move_path[i]);
}

struct xy render_point(float u, float per_circle) {
	double ipart;
	u = modf(u, &ipart);
	u *= 12.0;
	float fpart = modf(u, &ipart);
	int i = ipart;
	return nurbs_evaluate(moving_circles[i], fmod(u * per_circle / 12.0, 1.0), fpart);
}
