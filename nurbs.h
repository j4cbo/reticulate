#ifndef NURBS_H
#define NURBS_H

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
	struct nurbs_point *t[4];
	float *knots;
};

struct xy nurbs_evaluate(const struct nurbs_patch *p, float u, float v);

#endif
