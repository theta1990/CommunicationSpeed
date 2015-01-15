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
#include "../murmur_hash.h"
const int Prime = 1024*1024*1024 + 1;

void permuate1(uint32_t *array, uint32_t size) {
	uint32_t i;
	for (i = 0; i < size; ++i) {
		array[i] = (rand()) % size;
	}
}

/**
 * memory access thread
 */
void *memthread(void *arg) {

	uint32_t *ptr, *remotePtr;
	uint64_t begin, end;
	uint64_t aggre = 0;
//	const int32_t size = 1024 * 1024 * 128;
	const int32_t size = Prime;
	const int32_t iter = size / 1024;
	uint32_t i, j;
	if (numa_run_on_node(0) < 0) {
		VOLT_WARN("bind failure");
	} else {

		ptr = (uint32_t *) numa_alloc_onnode(sizeof(uint32_t) * Prime, 0);

		permuate1(ptr, size);

		aggre = 0;
		begin = common::getCycleCount();
		j = 0;
		for (i = 0; i < iter; ++i) {
			j = ptr[j];
		}
		end = common::getCycleCount();
		VOLT_INFO("local access cost: \t%ld", (end - begin) / iter, j);

		remotePtr = (uint32_t *) numa_alloc_onnode(sizeof(uint32_t) * Prime, 1);

//		permuate1(remotePtr, size);
		memcpy(remotePtr, ptr, size * sizeof(uint32_t));

		aggre = 0;
		begin = common::getCycleCount();
		j = 0;
		for (i = 0; i < iter; ++i) {
			j = remotePtr[j];
		}

		end = common::getCycleCount();

		VOLT_INFO("remote access, cost: %ld", (end - begin) / iter, j);
	}
	return NULL;
}

/*
 * cas test
 */
void *casthread(void *arg) {

	uint32_t *ptr, *remotePtr;
	uint64_t begin, end;
	uint64_t aggre = 0;
//	const int32_t size = 1024 * 1024 * 128;
	const int32_t size = Prime;
	const int32_t iter = size / 1024;
	uint32_t i, j;
	if (numa_run_on_node(0) < 0) {
		VOLT_WARN("bind failure");
	} else {

		ptr = (uint32_t *) numa_alloc_onnode(sizeof(uint32_t) * Prime, 0);

		permuate1(ptr, size);

		aggre = 0;
		begin = common::getCycleCount();
		volatile int length=16;
		for (i = 0; i < iter; i+=length) {
//			j = ptr[i];
//			aggre += ptr[j];
			atomic::atomic_inc(&ptr[i%(2048*1024*1024)]);
//			ptr[j] --;

//			atomic::atomic_dec(&ptr[j]);
		}
		end = common::getCycleCount();
		VOLT_INFO("local access cost: \t%ld", (end - begin) / iter);

		remotePtr = (uint32_t *) numa_alloc_onnode(sizeof(uint32_t) * Prime, 1);

//		permuate1(remotePtr, size);
		memcpy(remotePtr, ptr, size * sizeof(uint32_t));

		aggre = 0;
		begin = common::getCycleCount();

		for (i = 0; i < iter; i+=length) {
//			j = remotePtr[i];
			atomic::atomic_inc(&ptr[i%(2048*1024*1024)]);
//			ptr[j] --;
//			atomic::atomic_dec(&ptr[j]);
		}

		end = common::getCycleCount();

		VOLT_INFO("remote access, cost: %ld", (end - begin)/iter);
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
//
		printf("----------cas memory access----------\r\n");
//		casthread(&node);

//		printf("----------barrier memeory access--------\r\n");

//		mem1 = (int*) malloc(sizeof(int));
//		mem2 = (int*) malloc(sizeof(int));
//
//		pthread_t pid1, pid2;
//		pthread_create(&pid1, NULL, barrierThread1, NULL);
//		pthread_create(&pid2, NULL, barrierThread2, NULL);
//
//		pthread_join(pid1, NULL);
//		pthread_join(pid2, NULL);
	}

	return 0;
}
//TEST(numatest);

