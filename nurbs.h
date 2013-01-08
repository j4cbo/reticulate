#ifndef NURBS_H
#define NURBS_H

#define NURBS_T_POINTS 4

struct xy {
	float x;
	float y;
};

struct nurbs_point {
	float x;
	float y;
	float weight;
};

struct nurbs_patch {
	int points;
	const float *xy_knots;
	const float *t_knots;
	struct nurbs_point t[][NURBS_T_POINTS];
};

struct xy nurbs_evaluate(const struct nurbs_patch *p, float u, float v);

#endif
