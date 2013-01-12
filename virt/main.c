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

#define WINDOW_SIZE		600

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

/* render()
 *
 * Render all buffered points to the screen.
 */
static void render(void) {
	/* Start with the oldest frame, up through newest */
	int next = (s.store_frame + 1) % POV_FRAMES;

	/* Draw all active points */
	for (int i = 0; i < POV_FRAMES; i++) {
		struct frame *f = &s.frames[(next + i) % POV_FRAMES];

		const int stride = sizeof (struct dac_point);
		glColorPointer(3, GL_UNSIGNED_SHORT, stride, &f->data->r);
		glVertexPointer(2, GL_SHORT, stride, &f->data->x);
		glDrawArrays(GL_LINE_STRIP, 0, f->size); 
	}

	/* Advance the frame buffer */
	s.frames[next].size = 0;
	s.frames[next].last_copy = s.frames[s.store_frame].last_copy;
	s.store_frame = next;
}

/* fill_status(status)
 *
 * Fill in a struct dac_status with the current state of things.
 */
void fill_status(struct dac_status *status) {
	memset(status, 0, sizeof *status);
	status->playback_state = s.dac_state;
	status->buffer_fullness = (s.point_buf_produce - s.point_buf_consume
	                          + DAC_BUFFER_POINTS) % DAC_BUFFER_POINTS;
	status->point_rate = (s.dac_state == DAC_RUNNING) ? 30000 : 0;
	status->point_count = s.point_count;
}

/* broadcast_thread_func(arg)
 *
 * Send out periodic broadcasts announcing our presence.
 */
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

/* check_write(fd, vbuf, len)
 *
 * Write all of vbuf to a socket and assert that the write succeeds.
 */
static void check_write(int fd, const void *vbuf, size_t len) {
	const char *buf = vbuf;
	while (len) {
		int res = write(fd, buf, len);
		CHK("write", res);
		len -= res;
		buf += res;
	}
}

/* read_exactly(fd, vbuf, len)
 *
 * Read exactly len bytes into buf.
 */
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

/* send_resp
 *
 * Send a response (ACK or NAK, and status) back to the client.
 */
static void send_resp(int fd, char resp, char cmd) {
	struct dac_response response = { resp, cmd };
	fill_status(&response.dac_status);
	check_write(fd, &response, sizeof response);
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

static const char version_string[32] = "simulator";

/* read_and_process_command(fd)
 *
 * Read a command from the network and process it.
 */
static int process_command(int fd) {
	char buf[6];
	char cmd;
	if (read_exactly(fd, &cmd, sizeof cmd) < 0)
		return -1;

	HOLDING(&s_lock)
		copy_points_into_frame();

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
		CHK("accept", fd);
		printf("Connection from %s\n", inet_ntoa(client.sin_addr));

		/* Send initial status response */
		send_resp(fd, RESP_ACK, '?');

		while (process_command(fd) == 1);

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
	glOrtho(-32769, 32768, -32769, 32768, -1.0f, 1.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glEnableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);

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

	long long t = microseconds();
	int frames = 0;

	long long next_frame = microseconds();

	while (!SDL_QuitRequested()) {

		/* Run the SDL event loop */
		SDL_Event event;
		while (SDL_PollEvent(&event));

		/* Clear the screen and render the next frame */
		glClear(GL_COLOR_BUFFER_BIT);
		HOLDING(&s_lock)
			render();

		long long now = microseconds();
		frames++;
		if (t + 1000000 < now) {
			char buf[20];
			sprintf(buf, "%d FPS", frames);
			SDL_WM_SetCaption(buf, 0);
			printf("%s\n", buf);
			t += 1000000;
			frames = 0;
		}

		SDL_GL_SwapBuffers();

		/* Delay if needed */
		next_frame += 1000000 / FPS;
		long long delay_time = (next_frame - microseconds()) / 1000;
		if (delay_time > 0)
			SDL_Delay(delay_time);
	}

	return 0;
}
