#include "permutate.h"
#include "../debuglog.h"
#include <assert.h>
#include <stdlib.h>
const int32_t CacheLineSize = 64;
const int32_t L1CacheSize = 32 * 1024;
const int32_t L2CacheSize = 256 * 1024;
const int32_t L3CacheSize = 16 * 1024 * 1024;

/**
 * pattern:
 * 0 1 2 ... 63 | 64 65 ... 127 |
 * 63 0 ... 1 2 | 127 ... 65 64 |
 */
int32_t l1h(uint32_t* array, uint32_t size) {

	const int32_t wordCount = CacheLineSize / sizeof(uint32_t);
	const int32_t range = L1CacheSize / sizeof(uint32_t);

	assert(range < size);

	for (uint32_t i = 0; i < range; ++i) {

		array[i] = rand() % range;	//保证落在range中
	}
	return wordCount;
}

void warmUpL1(uint32_t* array, uint32_t size) {

	const int32_t range = L1CacheSize / sizeof(uint32_t);
	uint32_t i, j;

	if (range > size) {

		VOLT_ERROR("Array size is less than L1 cache");
	} else {

		for (i = 0, j = 0; i < range; ++i) {
			j = array[j];
		}
		printf("", j);
	}
}

/**
 * pattern:
 * 0 1 2 ... 63 | 64 65 ... 127 | 128 ... |
 * 128 ........ | 256 ....      | 64 .... |
 */
int32_t l1ml2h(uint32_t* array, uint32_t size) {

	const int32_t step = CacheLineSize / sizeof(uint32_t);
	const int32_t range = L2CacheSize / sizeof(uint32_t);

	assert(range < size);

	for (uint32_t i = 0; i < range; i += step) {

		array[i] = (i + step * 5) % range;	//以2倍的迭代遍历L2 cache，再迭代完1遍L2 cache时， L1 cache已经被刷了4次了， 因为L2 大小是L1大小的8倍，而我们实际访问了L2 cache的一半内容
	}
	return range;
}

void warmUpL2(uint32_t* array, uint32_t size) {

	const int32_t range = L2CacheSize / sizeof(uint32_t);
	uint32_t i, j;

	if (range > size) {
		VOLT_ERROR("Array size is less than L2 cache");
	} else {

		for (i = 0, j = 0; i < range; ++i) {
			j = array[j];
		}
		printf("", j);
	}
}

/**
 * L3 cache是多个core共用的？我觉得实际上我们只能保证占用L3 cache的一部分空间
 */
int32_t l2ml3h(uint32_t* array, uint32_t size) {

	const int32_t step = CacheLineSize / sizeof(uint32_t);
	const int32_t range = L3CacheSize / sizeof(uint32_t);

	assert(range < size);

	for(int32_t i = 0; i < range; i += step){

		array[i] = (i + step * 5) % range; //以k倍速度遍历L3 cache， 遍历完时， L2 cache被刷新了 64 / k次， 基本上能保证新的一轮迭代开始的时候，又会发生cache miss
	}
	return range;
}

void warmUpL3(uint32_t* array, uint32_t size) {

	const int32_t range = L3CacheSize / sizeof(uint32_t);
	uint32_t i, j;

	if (range > size) {
		VOLT_ERROR("Array size is less than L2 cache");
	} else {

		for (i = 0, j = 0; i < range; ++i) {
			j = array[j];
		}
		printf("", j);
	}
}

/**
 * L3 cache miss
 */
int32_t l3m(uint32_t* array, uint32_t size) {

	const int32_t step = CacheLineSize / sizeof(uint32_t);
	const int32_t range = L3CacheSize * 32 / sizeof(uint32_t);

	assert(range < size);

	for (int32_t i = 0; i < range; i += step) {

		array[i] = (i + step * 5) % range;
	}
	return range;
}

void warmUpL4(uint32_t* array, uint32_t size){

	//实际上是要clean掉所有的cache信息
	uint32_t j = 0;
	for(uint32_t i = 0; i < size; ++i){
		j = array[j];
	}
}
