/*
 * numatest.cpp
 *
 *  Created on: Jan 9, 2015
 *      Author: volt
 */

#include <numa.h>
#include <sched.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include "../atomic.h"
#include "../test.h"
#include "../clock.h"
#include "../murmur_hash.h"
#include "permutate.h"
#include "../pcm/cpucounters.h"
const int Prime = 1024 * 1024 * 1024 + 1;
int ITERCASE = 10240000;

void permuate1(uint32_t *array, uint32_t size) {
	uint32_t i;
	for (i = 0; i < size; ++i) {
		array[i] = (rand()) % size;
	}
}

inline int32_t setAffinity(int32_t id) {

	cpu_set_t set;
	CPU_ZERO(&set);
	CPU_SET(id, &set);
	return sched_setaffinity(getpid(), 1, &set);
}

inline void outputDetailStatistic(CoreCounterState before_sstate,
		CoreCounterState after_sstate) {

	std::cout << "L2 cache hit ratio:"
			<< getL2CacheHitRatio(before_sstate, after_sstate)<<std::endl
			<< "L3 cache hit ratio:"
			<< getL3CacheHitRatio(before_sstate, after_sstate)<<std::endl;
}

void testCache() {

}

/**
 * memory access thread
 */
void *memthread(void *arg) {

	uint32_t *ptr, *remotePtr;
	uint64_t begin, end;
//	uint64_t aggre = 0;
//	const int32_t size = 1024 * 1024 * 128;
//	const int32_t size = Prime;
	const int32_t size = L3CacheSize * 32;
	const uint32_t iter = ITERCASE;
	uint32_t i, j;
	uint32_t range;

	int coreId = 0;

//	if (numa_run_on_node(0) < 0) {

	PCM * m = PCM::getInstance();
	CoreCounterState before_sstate;
	CoreCounterState after_sstate;
//	 program counters, and on a failure just exit

	m->cleanup();
	if (m->program() != PCM::Success) {
		VOLT_WARN("pcm open failed");
//		return NULL;
	}

	if (setAffinity(coreId) < 0) {
		VOLT_WARN("bind failure");
	} else {

		ptr = (uint32_t *) numa_alloc_onnode(sizeof(uint32_t) * size, 0);

//		permuate1(ptr, size);
		range = l1h(ptr, size);

//		aggre = 0;
		j = 0;
		warmUpL1(ptr, size);
		before_sstate = getCoreCounterState(coreId);
		begin = common::getCycleCount();
		for (i = 0; i < iter; ++i) {
			j = ptr[j];
		}
		end = common::getCycleCount();
		after_sstate = getCoreCounterState(coreId);
		VOLT_INFO("local L1 access cost: \t%ld", (end - begin) / iter, j);
		outputDetailStatistic(before_sstate, after_sstate);

		range = l1ml2h(ptr, size);
		j = 0;
		warmUpL2(ptr, size);
		before_sstate = getCoreCounterState(coreId);
		begin = common::getCycleCount();
		for (i = 0; i < iter; ++i) {
			j = ptr[j];
		}
		end = common::getCycleCount();
		after_sstate = getCoreCounterState(coreId);
		VOLT_INFO("local L2 access cost: \t%ld", (end - begin) / iter, j);
		outputDetailStatistic(before_sstate, after_sstate);

		range = l2ml3h(ptr, size);
		j = 0;
		warmUpL3(ptr, size);
		before_sstate = getCoreCounterState(coreId);
		begin = common::getCycleCount();
		for (i = 0; i < iter; ++i) {
			j = ptr[j];
		}
		end = common::getCycleCount();
		after_sstate = getCoreCounterState(coreId);
		VOLT_INFO("local L3 access cost: \t%ld", (end - begin) / iter, j);
		outputDetailStatistic(before_sstate, after_sstate);

		range = l3m(ptr, size);
		j = 0;
		warmUpL4(ptr, size);
		before_sstate = getCoreCounterState(coreId);
		begin = common::getCycleCount();
		for (i = 0; i < iter; ++i) {
			j = ptr[j];
		}
		end = common::getCycleCount();
		after_sstate = getCoreCounterState(coreId);
		VOLT_INFO("local me access cost: \t%ld", (end - begin) / iter, j);
		outputDetailStatistic(before_sstate, after_sstate);//		numaNodes = numa_max_node();
		//		VOLT_INFO("numa node info");
		//		for (int i = 0; i <= numaNodes; ++i) {
		//
		//			size = numa_node_size(i, &freep);
		//			VOLT_INFO("node: %d, size: %ld\n", i, size);
		//		}

		numa_free(ptr, size);
		ptr = NULL;

		ptr = (uint32_t *) numa_alloc_onnode(sizeof(uint32_t) * size, 1);

//		permuate1(ptr, size);
		range = l1h(ptr, size);
		j = 0;
		warmUpL1(ptr, size);
		before_sstate = getCoreCounterState(coreId);
		begin = common::getCycleCount();
		for (i = 0; i < iter; ++i) {
			j = ptr[j];
		}
		end = common::getCycleCount();
		after_sstate = getCoreCounterState(coreId);
		VOLT_INFO("remote L1 access cost: \t%ld", (end - begin) / iter, j);
		outputDetailStatistic(before_sstate, after_sstate);


		range = l1ml2h(ptr, size);
		j = 0;
		warmUpL2(ptr, size);
		before_sstate = getCoreCounterState(coreId);
		begin = common::getCycleCount();
		for (i = 0; i < iter; ++i) {
			j = ptr[j];
		}
		end = common::getCycleCount();
		after_sstate = getCoreCounterState(coreId);
		VOLT_INFO("remote L2 access cost: \t%ld", (end - begin) / iter, j);
		outputDetailStatistic(before_sstate, after_sstate);

		range = l2ml3h(ptr, size);
		j = 0;
		warmUpL3(ptr, size);
		before_sstate = getCoreCounterState(coreId);
		begin = common::getCycleCount();
		for (i = 0; i < iter; ++i) {
			j = ptr[j];
		}
		end = common::getCycleCount();
		after_sstate = getCoreCounterState(coreId);
		VOLT_INFO("remote L3 access cost: \t%ld", (end - begin) / iter, j);
		outputDetailStatistic(before_sstate, after_sstate);

		range = l3m(ptr, size);
		j = 0;
		warmUpL4(ptr, size);
		before_sstate = getCoreCounterState(coreId);
		begin = common::getCycleCount();
		for (i = 0; i < iter; ++i) {
			j = ptr[j];
		}
		end = common::getCycleCount();
		after_sstate = getCoreCounterState(coreId);
		VOLT_INFO("remote me access cost: \t%ld", (end - begin) / iter, j);
		outputDetailStatistic(before_sstate, after_sstate);

		numa_free(ptr, size);

	}
	return NULL;
}

/*
 * cas test
 */
void *casthread(void *arg) {

	uint32_t *ptr;
	uint64_t begin, end;
//	uint64_t aggre = 0;
//	const int32_t size = 1024 * 1024 * 128;
//	const int32_t size = Prime;
	const int32_t size = L3CacheSize * 32;
	const uint32_t iter = ITERCASE;
	uint32_t i, j;
	uint32_t range;
	int coreId = 0;


	CoreCounterState beginState;
	CoreCounterState endState;

	PCM *m = PCM::getInstance();

	m->cleanup();
	if( m->program() != PCM::Success ) {
		VOLT_WARN("PCM inits not properly");
	}

	if (setAffinity(coreId) < 0) {
		VOLT_WARN("bind failure");
	} else {

		ptr = (uint32_t *) numa_alloc_onnode(sizeof(uint32_t) * size, 0);

//		permuate1(ptr, size);
		range = l1h(ptr, size);

//		aggre = 0;
		j = 0;
		warmUpL1(ptr, size);
		beginState = getCoreCounterState(coreId);
		begin = common::getCycleCount();
		for (i = 0; i < iter; ++i) {
			atomic::atomic_inc(&ptr[j]);
			--ptr[j];
			j = ptr[j];
		}
		end = common::getCycleCount();
		endState = getCoreCounterState(coreId);
		VOLT_INFO("local L1 access cost: \t%ld", (end - begin) / iter, j);
		outputDetailStatistic(beginState, endState);

		range = l1ml2h(ptr, size);
		j = 0;
		warmUpL2(ptr, size);
		beginState = getCoreCounterState(coreId);
		begin = common::getCycleCount();
		for (i = 0; i < iter; ++i) {
			atomic::atomic_inc(&ptr[j]);
			--ptr[j];
			j = ptr[j];
		}
		end = common::getCycleCount();
		endState = getCoreCounterState(coreId);
		VOLT_INFO("local L2 access cost: \t%ld", (end - begin) / iter, j);
		outputDetailStatistic(beginState, endState);

		range = l2ml3h(ptr, size);
		j = 0;
		warmUpL3(ptr, size);
		beginState = getCoreCounterState(coreId);
		begin = common::getCycleCount();

		for (i = 0; i < iter; ++i) {
			atomic::atomic_inc(&ptr[j]);
			--ptr[j];
			j = ptr[j];
		}
		end = common::getCycleCount();
		endState = getCoreCounterState(coreId);
		VOLT_INFO("local L3 access cost: \t%ld", (end - begin) / iter, j);
		outputDetailStatistic(beginState, endState);

		range = l3m(ptr, size);
		j = 0;
		warmUpL4(ptr, size);
		beginState = getCoreCounterState(coreId);
		begin = common::getCycleCount();

		for (i = 0; i < iter; ++i) {
			atomic::atomic_inc(&ptr[j]);
			--ptr[j];
			j = ptr[j];
		}
		end = common::getCycleCount();
		endState = getCoreCounterState(coreId);
		VOLT_INFO("local me access cost: \t%ld", (end - begin) / iter, j);
		outputDetailStatistic(beginState, endState);

		numa_free(ptr, size);
		ptr = NULL;

		ptr = (uint32_t *) numa_alloc_onnode(sizeof(uint32_t) * size, 1);

//		permuate1(ptr, size);
		range = l1h(ptr, size);

//		aggre = 0;
		j = 0;
		warmUpL1(ptr, size);
		beginState = getCoreCounterState(coreId);
		begin = common::getCycleCount();

		for (i = 0; i < iter; ++i) {
			atomic::atomic_inc(&ptr[j]);
			--ptr[j];
			j = ptr[j];
		}
		end = common::getCycleCount();
		endState = getCoreCounterState(coreId);
		VOLT_INFO("remote L1 access cost: \t%ld", (end - begin) / iter, j);
		outputDetailStatistic(beginState, endState);

		range = l1ml2h(ptr, size);
		j = 0;
		warmUpL2(ptr, size);
		beginState = getCoreCounterState(coreId);
		begin = common::getCycleCount();
		for (i = 0; i < iter; ++i) {
			atomic::atomic_inc(&ptr[j]);
			--ptr[j];
			j = ptr[j];
		}
		end = common::getCycleCount();
		endState = getCoreCounterState(coreId);
		VOLT_INFO("remote L2 access cost: \t%ld", (end - begin) / iter, j);
		outputDetailStatistic(beginState, endState);

		range = l2ml3h(ptr, size);
		j = 0;
		warmUpL3(ptr, size);
		beginState = getCoreCounterState(coreId);
		begin = common::getCycleCount();
		for (i = 0; i < iter; ++i) {
			atomic::atomic_inc(&ptr[j]);
			--ptr[j];
			j = ptr[j];
		}
		end = common::getCycleCount();
		endState = getCoreCounterState(coreId);
		VOLT_INFO("remote L3 access cost: \t%ld", (end - begin) / iter, j);
		outputDetailStatistic(beginState, endState);

		range = l3m(ptr, size);
		j = 0;
		warmUpL4(ptr, size);
		beginState = getCoreCounterState(coreId);
		begin = common::getCycleCount();
		for (i = 0; i < iter; ++i) {
			atomic::atomic_inc(&ptr[j]);
			--ptr[j];//		numaNodes = numa_max_node();
			//		VOLT_INFO("numa node info");
			//		for (int i = 0; i <= numaNodes; ++i) {
			//
			//			size = numa_node_size(i, &freep);
			//			VOLT_INFO("node: %d, size: %ld\n", i, size);
			//		}
			j = ptr[j];
		}
		end = common::getCycleCount();
		endState = getCoreCounterState(coreId);
		VOLT_INFO("remote me access cost: \t%ld", (end - begin) / iter, j);
		outputDetailStatistic(beginState, endState);

		numa_free(ptr, size);

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

	char ch;
	while (-1 != (ch = getopt(argc, argv, "c:"))) {
		switch (ch) {
		case 'c':
			ITERCASE = strtoll(optarg, NULL, 10);
			break;
		default:
			VOLT_ERROR("not support, usage: -c %d")
			;
		}
	}

	if (-1 == numa_available()) {
		VOLT_WARN("numa lib initilization failed");
	} else {

		int node = 0;

		printf("----------direct memory access----------\r\n");
		memthread(&node);
//
		printf("----------cas memory access----------\r\n");
		casthread(&node);

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
TEST(numatest);

