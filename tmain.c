#define _GNU_SOURCE

#include <math.h>
#include <stdio.h>
#include <unistd.h>

#include "etherdream.h"
#include "render.h"

#define PPS             30000
#define PATTERN_SECONDS 5
#define REFRESH_HZ	50

#define PATTERN_POINTS  (PPS * PATTERN_SECONDS)
#define PER_FRAME       1000

int main() {
	render_init();
	etherdream_lib_start();

	/* Sleep for a bit over a second, to ensure that we see all DACs */
	usleep(1200000);

	int dac_count = etherdream_dac_count();
	if (!dac_count) {
		printf("No DACs found.\n");
		return 0;
	}

	for (int i = 0; i < dac_count; i++)
		printf("DAC %d: Ether Dream %06lx\n", i,
		       etherdream_get_id(etherdream_get(i)));

	struct etherdream *d = etherdream_get(0);

	printf("Connecting...\n");
	if (etherdream_connect(d) < 0)
		return 1;

	int p = 0;
	while (1) {
		struct etherdream_point buf[PER_FRAME];

		for (int i = 0; i < PER_FRAME; i++) {
			render_point(
				&buf[i],
				(float)p / PATTERN_POINTS,
				PATTERN_SECONDS * REFRESH_HZ
			);

			p = (p + 1) % PATTERN_POINTS;
		}

		int res = etherdream_write(d, buf, PER_FRAME, PPS, 1);
		if (res != 0)
			printf("write %d\n", res);

		etherdream_wait_for_ready(d);
	}

	return 0;
}
