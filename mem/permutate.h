/*
 * permutate.h
 *
 *  Created on: Jan 14, 2015
 *      Author: volt
 */

#ifndef PERMUTATE_H_
#define PERMUTATE_H_

#include <stdint.h>

extern const int32_t CacheLineSize;
extern const int32_t L1CacheSize;
extern const int32_t L2CacheSize;
extern const int32_t L3CacheSize;

uint32_t l1h(uint32_t* array, uint32_t size);
uint32_t l1ml2h(uint32_t* array, uint32_t size);
uint32_t l2ml3h(uint32_t* array, uint32_t size);
uint32_t l3m(uint32_t* array, uint32_t size);

void warmUpL1(uint32_t* array, uint32_t size);
void warmUpL2(uint32_t* array, uint32_t size);
void warmUpL3(uint32_t* array, uint32_t size);
void warmUpL4(uint32_t* array, uint32_t size);


#endif /* PERMUTATE_H_ */
