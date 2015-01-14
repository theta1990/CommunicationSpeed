/*
 * clock.h
 *
 *  Created on: Dec 5, 2014
 *      Author: volt
 */

#ifndef CLOCK_H_
#define CLOCK_H_
#include <sys/time.h>
namespace common {
inline unsigned long long getCycleCount() {
	unsigned long long hi, lo;
	__asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
	return ((unsigned long long) lo) | (((unsigned long long) hi) << 32);
}

inline unsigned long long getTimeOfMs(){

	struct timeval val;
	gettimeofday(&val, NULL);
	return val.tv_sec * 1000000 + val.tv_usec;
}

}

#endif /* CLOCK_H_ */
