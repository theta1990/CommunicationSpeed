/*
 * numatest.cpp
 *
 *  Created on: Jan 9, 2015
 *      Author: volt
 */

#include <numa.h>

#include <stdint.h>
#include "../atomic.h"
#include "../test.h"
#include "../clock.h"

/**
 * memory access thread
 */
void *memthread(void *arg) {

	int node = *static_cast<int*>(arg);
	uint8_t *ptr, *remotePtr;
	uint64_t begin, end;
	if (numa_run_on_node(node) < 0) {
		VOLT_WARN("bind failure");
	} else {

		ptr = (uint8_t *) numa_alloc_onnode(sizeof(uint8_t), node);

		begin = common::getCycleCount();
		for (int i = 0; i < 1000; ++i) {
			(*ptr)++;
		}
		end = common::getCycleCount();
		VOLT_INFO("local access cost: %ld", end - begin);

		remotePtr = (uint8_t *) numa_alloc_onnode(sizeof(uint8_t),
				(node + 1) % 2);
		*remotePtr = 0;
		begin = common::getCycleCount();
		for (int i = 0; i < 1000; ++i) {
			(*remotePtr)++;
		}
		end = common::getCycleCount();

		VOLT_INFO("remote access, cost: %ld", end - begin);
	}
	return NULL;
}

/*
 * cas test
 */
void *casthread(void *arg) {

	int node = *static_cast<int*>(arg);
	uint8_t *ptr, *remotePtr;
	uint64_t begin, end;
	if (numa_run_on_node(node) < 0) {
		VOLT_WARN("bind failure");
	} else {

		ptr = (uint8_t *) numa_alloc_onnode(sizeof(uint8_t), node);

		begin = common::getCycleCount();
		for (int i = 0; i < 1000; ++i) {
			atomic::atomic_inc(ptr);
		}
		end = common::getCycleCount();
		VOLT_INFO("local access cost: %ld", end - begin);

		remotePtr = (uint8_t *) numa_alloc_onnode(sizeof(uint8_t),
				(node + 1) % 2);
		*remotePtr = 0;
		begin = common::getCycleCount();
		for (int i = 0; i < 1000; ++i) {
			atomic::atomic_inc(remotePtr);
		}
		end = common::getCycleCount();

		VOLT_INFO("remote access, cost: %ld", end - begin);
	}
	return NULL;
}

int *mem1;
int *mem2;

void *barrierThread1(void *arg) {

	*mem1 = 1;
	*mem2 = 2;

	if (numa_run_on_node(0) < 0) {
		VOLT_WARN("bind failure");
	} else {
		while (true) {

			(*mem1) += 10;
			(*mem2) += 10;
		}
	}
	return NULL;
}

void *barrierThread2(void *arg) {

	int x, y;
	if (numa_run_on_node(1) < 0) {
		VOLT_WARN("bind failure");
	} else {
		while (true) {

			x = (*mem1);
			y = (*mem2);
			VOLT_INFO("x = %d, y = %d", x, y);
		}
	}
	return NULL;
}

int numatest(int argc, char **argv) {

//	int numaNodes;
//	long int freep;
//	long size;
	if (-1 == numa_available()) {
		VOLT_WARN("numa lib initilization failed");
	} else {

//		numaNodes = numa_max_node();
//		VOLT_INFO("numa node info");
//		for (int i = 0; i <= numaNodes; ++i) {
//
//			size = numa_node_size(i, &freep);
//			VOLT_INFO("node: %d, size: %ld\n", i, size);
//		}
		int node = 0;

		printf("----------direct memory access----------\r\n");
		memthread(&node);

		printf("----------remote memory access----------\r\n");
		casthread(&node);

		printf("----------barrier memeory access--------\r\n");

		pthread_t pid1, pid2;
		pthread_create(&pid1, NULL, barrierThread1, NULL);
		pthread_create(&pid2, NULL, barrierThread2, NULL);


		pthread_join(pid1, NULL);
		pthread_join(pid2, NULL);
	}

	return 0;
}
TEST(numatest);

