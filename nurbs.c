#include "nurbs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct nurbs_line *nurbs_load_line(const char *filename) {
	struct nurbs_line *out = NULL;
	FILE *fp = fopen(filename, "r");
	if (!fp)
		return NULL;

	char buf[8];
	if (fread(buf, sizeof buf, 1, fp) != 1) {
		printf("not a nub3 file\n");
		goto bail;
	}

	if (memcmp(buf, "nub\003", 4)) {
		printf("not a nub3 file\n");
		goto bail;
	}

	int count = *(int *)(buf + 4);
	printf("points: %d\n", count);

	out = malloc(sizeof *out + (count * sizeof (struct nurbs_point)
	                         + ((count + 3) * sizeof (float))));
	if (!out) {
		printf("oom in nurbs_load_line\n");
		goto bail;
	}

	out->points = count;
	out->knots = (float *)(out->t + count);

	if (fread(out->t, sizeof (float), (count * 4) + 3, fp) != (count * 4) + 3) {
		printf("short read\n");
		goto bail;
	}

	fclose(fp);
	return out;

bail:
	fclose(fp);
	free(out);
	return NULL;
}

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

struct xy nurbs_evaluate(const struct nurbs_patch *p, float u, float v) {
	struct xy out = { 0, 0 };
	float div = 0;

	for (int i = 0; i < p->points; i++) {
		for (int j = 0; j < NURBS_T_POINTS; j++) {
			float r = nurbs_N2(p->xy_knots, i, u);
			if (!r)
				continue;

			r *= nurbs_N2(p->t_knots, j, v);
			if (!r)
				continue;

			r *= p->t[i][j].weight;

			div += r;
			out.x += r * p->t[i][j].x;
			out.y += r * p->t[i][j].y;
		}
	}

	out.x /= div;
	out.y /= div;

	return out;
}
