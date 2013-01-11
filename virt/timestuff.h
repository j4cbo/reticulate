/* timestuff.h: time functions from Ether Dream interface library
 *
 * Copyright 2011-2012 Jacob Potter
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License version 2
 * or 3, or the GNU Lesser General Public License version 3, as published
 * by the Free Software Foundation, at your option.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TIMESTUFF_H
#define TIMESTUFF_H

#ifdef __MACH__
#include <mach/mach.h>
#include <mach/mach_time.h>
static long long timer_start, timer_freq_numer, timer_freq_denom;
#else
static struct timespec start_time;
#endif

/* microseconds()
 *
 * Return the number of microseconds since initialization.
 */
static inline long long microseconds(void) {
#if __MACH__
	long long time_diff = mach_absolute_time() - timer_start;
	return time_diff * timer_freq_numer / timer_freq_denom;
#else
	struct timespec t;
	clock_gettime(CLOCK_REALTIME, &t);
	return (t.tv_sec - start_time.tv_sec) * 1000000 +
	       (t.tv_nsec - start_time.tv_nsec) / 1000;
#endif
}

/* microsleep(us)
 *
 * Like usleep().
 */
static inline void microsleep(long long us) {
	nanosleep(&(struct timespec){ .tv_sec = us / 1000000,
	                             .tv_nsec = (us % 1000000) * 1000 }, NULL);
}

/* time_init()
 *
 * Set up whatever clock crap is needed.
 */
static inline void time_init(void) {
#if __MACH__
	timer_start = mach_absolute_time();
	mach_timebase_info_data_t timebase_info;
	mach_timebase_info(&timebase_info);
	timer_freq_numer = timebase_info.numer;
	timer_freq_denom = timebase_info.denom * 1000;
#else
	clock_gettime(CLOCK_REALTIME, &start_time);
#endif
}

#endif
