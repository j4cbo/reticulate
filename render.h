#ifndef RENDER_H
#define RENDER_H

#include "etherdream.h"
#include "nurbs.h"

void render_init(void);
void render_point(struct etherdream_point *pt, float u, float redraw_count);

#endif
