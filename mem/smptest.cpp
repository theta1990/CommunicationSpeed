/*
 * smptest.cpp
 *
 *  Created on: Jan 14, 2015
 *      Author: volt
 */

#include <stdint.h>
#include <set>
#include "../atomic.h"
#include "../test.h"
#include "../clock.h"
#include "../murmur_hash.h"
#include "permutate.h"
//extern void permuate1(uint32_t *array, uint32_t size);
/**
 * memory access thread
 */
void *smpmemthread() {

	uint32_t *ptr;
	uint64_t begin, end;
//	const int32_t size = 1024 * 1024 * 128;
	const int32_t size = 1024*1024*1024 + 1;
	const int32_t iter = size / 1024;
	int32_t i, j;

	ptr = (uint32_t *) malloc(sizeof(uint32_t) * size);


	begin = common::getCycleCount();
	for (i = 0; i < iter; ++i) {
		j = ptr[j];
	}
	end = common::getCycleCount();
	VOLT_INFO("local access cost: \t%ld", (end - begin) / iter, j);

	free(ptr);
	return NULL;
}

void *testPermutate(){

	uint32_t *array;
	int32_t range;
	uint32_t size = L3CacheSize * 32;
	uint32_t j = 0;
	array = (uint32_t *)malloc(sizeof(uint32_t) * L3CacheSize * 32);

	range = l1ml2h(array, size);

	std::set<uint32_t> set;
	set.clear();
	for(int32_t i = 0;i<range ; i+= CacheLineSize/sizeof(uint32_t)){
		set.insert(j);
		j = array[j];
	}
	printf("%d %d\n", range/(CacheLineSize/sizeof(uint32_t)), set.size());

	free(array);
	return NULL;
}


int smptest(int argc, char **argv){

//	smpmemthread();
	testPermutate();
	return 0;
}
TEST(smptest);
