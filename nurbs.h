#ifndef NURBS_H
#define NURBS_H

#define NURBS_T_POINTS 4

extern const float nurbs_t_knots[NURBS_T_POINTS + 3];

struct xy {
	float x;
	float y;
};

struct nurbs_point {
	float x;
	float y;
	float weight;
} __attribute__((packed));

struct nurbs_line {
	int points;
	const float *knots;
	struct nurbs_point t[];
} __attribute__((packed));

struct nurbs_patch {
	int points;
	const float *xy_knots;
	struct nurbs_point t[][NURBS_T_POINTS];
} __attribute__((packed));

struct nurbs_line *nurbs_load_line(const char *filename);
struct nurbs_patch *nurbs_extrude(const struct nurbs_line *line);

struct xy nurbs_evaluate(const struct nurbs_patch *p, float u, float v);

#endif
