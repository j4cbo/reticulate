#include "nurbs.h"

static inline float nurbs_f(const float *knots, int i, int n, float u) {
	return (u - knots[i]) / (knots[i + n] - knots[i]);
}

static inline float nurbs_g(const float *knots, int i, int n, float u) {
	return (knots[i + n] - u) / (knots[i + n] - knots[i]);
}

static inline float nurbs_N1(const float *knots, int i, float u) {
	float out = 0;
	if (knots[i] <= u && u < knots[i + 1])
		out = nurbs_f(knots, i, 1, u);
	if (knots[i + 1] <= u && u < knots[i + 2])
		out += nurbs_g(knots, i + 1, 1, u);
	return out;
}

static inline float nurbs_N2(const float *knots, int i, float u) {
	float out = 0;
	if (knots[i] <= u && u < knots[i + 2])
		out = nurbs_f(knots, i, 2, u) * nurbs_N1(knots, i, u);
	if (knots[i + 1] <= u && u < knots[i + 3])
		out += nurbs_g(knots, i + 1, 2, u) * nurbs_N1(knots, i + 1, u);
	return out;
}

static const float nurbs_t_knots[7] = { -1.5, -1.5, -.5, .5, 1.5, 2.5, 2.5 };

struct xy nurbs_evaluate(const struct nurbs_patch *p, float u, float v) {
	int i, j;
	struct xy out = { 0, 0 };
	float div = 0;

	for (j = 0; j < 4; j++) {
		for (i = 0; i < p->points; i++) {
 			float r = nurbs_N2(p->knots, i, u);
			if (!r)
				continue;

			r *= nurbs_N2(nurbs_t_knots, j, v);
			if (!r)
				continue;

			r *= p->t[j][i].weight;

			div += r;
			out.x += r * p->t[j][i].x;
			out.y += r * p->t[j][i].y;
		}
	}

	out.x /= div;
	out.y /= div;

	return out;
}
