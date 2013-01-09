#define _GNU_SOURCE

#include <math.h>
#include <stdio.h>
#include <unistd.h>

#include "etherdream.h"
#include "render.h"

#define PER_FRAME 600


#define PPS		30000
#define PATTERN_SECONDS 5
#define CIRCLE_HZ	50

#define PATTERN_POINTS (PPS * PATTERN_SECONDS)

struct etherdream_point buf[PER_FRAME];

int main() {
	render_init();
	etherdream_lib_start();

	/* Sleep for a bit over a second, to ensure that we see broadcasts
	 * from all available DACs. */
	usleep(1200000);

	int cc = etherdream_dac_count();
	if (!cc) {
		printf("No DACs found.\n");
		return 0;
	}

	int i;
	for (i = 0; i < cc; i++) {
		printf("%d: Ether Dream %06lx\n", i,
			etherdream_get_id(etherdream_get(i)));
	}

	struct etherdream *d = etherdream_get(0);

	printf("Connecting...\n");
	if (etherdream_connect(d) < 0)
		return 1;

	i = 0;
	int p = 0;
	while (1) {
		for (i = 0; i < PER_FRAME; i++) {
			struct etherdream_point *pt = &buf[i];
			float u = p / ((float)PATTERN_POINTS);
			struct xy xy = render_point(u, PATTERN_SECONDS * CIRCLE_HZ);
			pt->x = xy.x * 5000;
			pt->y = xy.y * 5000;
			pt->r = 65535;
			pt->g = 65535;
			pt->b = 65535;
			p++;
		}

		int res = etherdream_write(d, buf, PER_FRAME, PPS, 1);
		if (res != 0) {
			printf("write %d\n", res);
		}
		etherdream_wait_for_ready(d);
		i++;
	}

	printf("done\n");
	return 0;
}
