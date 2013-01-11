/* Ether Dream simulator
 *
 * Copyright 2013 Jacob Potter
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <arpa/inet.h>
#include <assert.h>
#include <math.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <unistd.h>
#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>

#include <protocol.h>

#include "timestuff.h"

#define WINDOW_SIZE		500
#define OVERSCAN		1.1

#define POINT_RATE		30000
#define PERSIST_MS		100

#define DAC_BUFFER_POINTS	1800

#define FPS			60
#define POV_FRAMES		(FPS * PERSIST_MS / 1000)
#define MAX_DATA_PER_FRAME	(POINT_RATE * 2 / FPS)

#define HOLDING(m) \
	for (int _guard = (pthread_mutex_lock(m), 0); !_guard; \
	     pthread_mutex_unlock(m)) while (!_guard++)

#define CHK(call_, e_) if ((e_) < 0) { perror(call_); exit(1); }
#define PACKED __attribute__((packed))
#define MIN(a_, b_) ((a_) < (b_) ? (a_) : (b_))

struct frame {
	struct dac_point data[MAX_DATA_PER_FRAME] PACKED;
	int size;
	long long last_copy;
};

struct {
	enum { DAC_IDLE = 0, DAC_PREPARED = 1, DAC_RUNNING = 2 } dac_state;

	struct dac_point point_buf[DAC_BUFFER_POINTS] PACKED;
	int point_buf_produce, point_buf_consume;

	struct frame frames[POV_FRAMES];
	int store_frame;

	int point_count;
} s;

pthread_mutex_t s_lock;

static void render(void) {
	glClear(GL_COLOR_BUFFER_BIT);

	glEnableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);

	HOLDING(&s_lock) {
		/* Start with the oldest frame, up through newest */
		int next = (s.store_frame + 1) % POV_FRAMES;

		/* Draw all active points */
		for (int i = 0; i < POV_FRAMES; i++) {
			struct frame *f = &s.frames[(next + i) % POV_FRAMES];

			glColorPointer(3, GL_UNSIGNED_SHORT,
			               sizeof (struct dac_point), &f->data->r);
			glVertexPointer(2, GL_SHORT,
			               sizeof (struct dac_point), &f->data->x);

			glDrawArrays(GL_LINE_STRIP, 0, f->size); 
		}

		/* Advance the frame buffer */
		s.frames[next].size = 0;
		s.frames[next].last_copy = s.frames[s.store_frame].last_copy;
		s.store_frame = next;
	}

	SDL_GL_SwapBuffers();
}

/* fill_status
 *
 * Fill in a struct dac_status with the current state of things.
 */
void fill_status(struct dac_status *status) {
	status->protocol = 0;
	status->light_engine_state = 0;
	status->playback_state = s.dac_state;

	int fullness = s.point_buf_produce - s.point_buf_consume;
	if (fullness < 0)
		fullness += DAC_BUFFER_POINTS;
	status->buffer_fullness = fullness;
	status->playback_flags = 0;
	status->light_engine_flags = 0;

	/* Only report a point rate if currently playing */
	if (status->playback_state == DAC_RUNNING)
		status->point_rate = 30000;
	else
		status->point_rate = 0;

	status->point_count = s.point_count;
	status->source = 0;
	status->source_flags = 0;
}

void *broadcast_thread_func(void *arg) {
	int udpfd = *(int *)arg;

	const struct sockaddr_in dest_addr = {
		.sin_family = AF_INET,
		.sin_addr.s_addr = htonl(INADDR_ANY),
		.sin_port = htons(7654)
	};

	while (1) {
		struct dac_broadcast broadcast = {
			.mac_address = { 0x55, 0x55, 0x55, 0x55, 0x55, 0x55 },
			.hw_revision = 4321,
			.sw_revision = 2,
			.buffer_capacity = DAC_BUFFER_POINTS,
			.max_point_rate = POINT_RATE,
		};

		HOLDING(&s_lock)
			fill_status(&broadcast.status);

		CHK("sendto", sendto(udpfd, &broadcast, sizeof broadcast, 0,
		                     (const struct sockaddr *)&dest_addr,
		                     sizeof dest_addr));

		sleep(1);
	}
}

static void check_write(int fd, const void *vbuf, size_t len) {
	const char *buf = vbuf;
	while (len) {
		int res = write(fd, buf, len);
		CHK("write", res);
		len -= res;
		buf += res;
	}
}

static int read_exactly(int fd, void *vbuf, size_t len) {
	char *buf = vbuf;
	while (len) {
		int res = read(fd, buf, len);
		if (res < 0) {
			perror("read");
			return -1;
		} else if (res == 0) {
			return -1;
		}

		len -= res;
		buf += res;
	}

	return 0;
}

static void send_resp(int fd, char resp, char cmd) {
	struct dac_response response;
	response.response = resp;
	response.command = cmd;
	fill_status(&response.dac_status);

	check_write(fd, &response, sizeof response);
}

static const char version_string[32] = "simulator";

static int process_command(int fd, char cmd) {
	char buf[6];

	switch (cmd) {
	case 'v':
		check_write(fd, version_string, sizeof version_string);
		break;

	case 'p':
		HOLDING(&s_lock) {
			if (s.dac_state == DAC_IDLE) {
				s.dac_state = DAC_PREPARED;
				send_resp(fd, RESP_ACK, cmd);
			} else {
				send_resp(fd, RESP_NAK_INVL, cmd);
			}
		}
		break;

	case 'q':
		/* Ignore the point_rate parameter */
		if (read_exactly(fd, buf, 4) < 0)
			return -1;

		send_resp(fd, RESP_ACK, cmd);
		break;

	case 'd':
		if (read_exactly(fd, buf, 2) < 0)
			return -1;

		uint16_t npoints = *(uint16_t *)buf;;

		/* Read the data */
		bool overflow = false;
		while (npoints--) {
			struct dac_point point;
			if (read_exactly(fd, &point, sizeof point) < 0)
				return -1;

			if (overflow)
				continue;

			HOLDING(&s_lock) {
				int next = (s.point_buf_produce + 1)
				         % DAC_BUFFER_POINTS;

				if (next == s.point_buf_consume) {
					overflow = true;
					break;
				}

				memcpy(s.point_buf + s.point_buf_produce,
				       &point, sizeof point);
				s.point_buf_produce = next;
			}
		}

		if (overflow)
			send_resp(fd, RESP_NAK_INVL, cmd);
		else
			send_resp(fd, RESP_ACK, cmd);

		break;

	case 'b':
		/* Ignore the low_water_mark and point_rate parameters */
		if (read_exactly(fd, buf, 6) < 0)
			return -1;

		if (s.dac_state == DAC_PREPARED)
			s.dac_state = DAC_RUNNING;
		else
			printf("dac: not starting - not prepared\n");

		send_resp(fd, RESP_ACK, cmd);
		break;

	default:
		printf("Bogus command %c\n", cmd);
		exit(0);
	}

	return 1;
}

static void copy_points_into_frame(void) {
	if (s.dac_state != DAC_RUNNING)
		return;

	struct frame *f = s.frames + s.store_frame;
	long long now = microseconds();

	int fullness = s.point_buf_produce - s.point_buf_consume;
	if (fullness < 0)
		fullness += DAC_BUFFER_POINTS;

	int n = MIN((now - f->last_copy) * POINT_RATE / 1000000,
	            MIN(fullness, MAX_DATA_PER_FRAME - f->size));

	int first = MIN(n, DAC_BUFFER_POINTS - s.point_buf_consume);
	
	memcpy(f->data + f->size, s.point_buf + s.point_buf_consume,
	       first * sizeof (struct dac_point));
	memcpy(f->data + f->size + first, s.point_buf,
	       (n - first) * sizeof (struct dac_point));

	s.point_count += n;
	s.point_buf_consume = (s.point_buf_consume + n) % DAC_BUFFER_POINTS;
	f->last_copy = now;
	f->size += n;

	if (f->size == MAX_DATA_PER_FRAME) {
		/* Advance the frame buffer */
		int next = (s.store_frame + 1) % POV_FRAMES;
		s.frames[next].size = 0;
		s.frames[next].last_copy = s.frames[s.store_frame].last_copy;
		s.store_frame = next;
	}
}

static void *net_thread_func(void *arg) {
	int srvfd = *(int *)arg;

	while (1) {
		/* Reset everything before the next connection */
		HOLDING(&s_lock)
			memset(&s, 0, sizeof s);

		struct sockaddr_in client = { 0 };
		socklen_t len = sizeof client;

		/* Wait for a connection */
		int fd = accept(srvfd, (struct sockaddr *)&client, &len);
		printf("Connection from %s\n", inet_ntoa(client.sin_addr));

		/* Send initial status response */
		send_resp(fd, RESP_ACK, '?');

		while (1) {
			char c;
			if (read_exactly(fd, &c, sizeof c) < 0)
				break;

			HOLDING(&s_lock)
				copy_points_into_frame();

			if (process_command(fd, c) != 1)
				break;
		}

		printf("Connection closed\n");
		close(fd);
	}
}

static int socket_bind_and_assert(int type, int port) {
	int fd = socket(PF_INET, type, 0);
	CHK("socket", fd);

	int opt = 1;
	CHK("SO_REUSEADDR", setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
	                               (const char *)&opt, sizeof opt));

	struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_addr.s_addr = htonl(INADDR_ANY),
		.sin_port = htons(port)
	};

	CHK("bind", bind(fd, (struct sockaddr *)&addr, sizeof addr));
	return fd;
}

SDL_Surface *video_init(int window_size) {
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
		exit(1);

	SDL_Surface *scr = SDL_SetVideoMode(window_size, window_size, 24,
	                                    SDL_OPENGL | SDL_GL_DOUBLEBUFFER);

	glEnable(GL_TEXTURE_2D);
	glViewport(0, 0, window_size, window_size);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-32767 * OVERSCAN, 32768 * OVERSCAN,
	        -32767 * OVERSCAN, 32768 * OVERSCAN, -1.0f, 1.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	return scr;
}

int main(int argc, char **argv) {
	time_init();

	CHK("pthread_mutex_init", pthread_mutex_init(&s_lock, NULL));

	pthread_t broadcast_thread, net_thread;

	int udpfd = socket_bind_and_assert(SOCK_DGRAM, 7655);
	int listenfd = socket_bind_and_assert(SOCK_STREAM, 7765);
	CHK("listen", listen(listenfd, 1));

	int res = pthread_create(&broadcast_thread, NULL, 
	                         broadcast_thread_func, &udpfd);
	assert(res == 0);
	res = pthread_create(&net_thread, NULL, net_thread_func, &listenfd);
	assert(res == 0);

	video_init(WINDOW_SIZE);

	long long next_frame = microseconds();

	while (!SDL_QuitRequested()) {
		/* Run the SDL event loop */
		SDL_Event event;
		while (SDL_PollEvent(&event));

		render();

		next_frame += 1000000 / FPS;
		long long delay_time = (next_frame - microseconds()) / 1000;
		if (delay_time > 0) {
			SDL_Delay(delay_time);
		}
	}

	return 0;
}
