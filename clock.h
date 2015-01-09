/*
 * clock.h
 *
 *  Created on: Dec 5, 2014
 *      Author: volt
 */

#ifndef CLOCK_H_
#define CLOCK_H_

namespace common {
inline unsigned long long getCycleCount() {
	unsigned long long hi, lo;
	__asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
	return ((unsigned long long) lo) | (((unsigned long long) hi) << 32);
}

}

#endif /* CLOCK_H_ */
