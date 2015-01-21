#ifndef MYPCM_H_
#define MYPCM_H_

#include "../pcm/cpucounters.h"

#define MEM_UOP_RETIRED				0xD0
#define LOADS						0x01

#define MEM_LOAD_UOPS_RETIRED_EVENT	0xD1
#define	L1_HIT						0x01
#define L2_HIT						0x02
#define LLC_HIT						0x04

class CoreState : public CoreCounterState {

	friend double getL1CacheHit(CoreState &a, CoreState &b);
	friend double getL2CacheHit(CoreState &a, CoreState &b);
	friend double getL3CacheHit(CoreState &a, CoreState &b);
	friend void dump(CoreState &a, CoreState &b);
public:
};

double getL1CacheHit(CoreState &a, CoreState &b);
double getL2CacheHit(CoreState &a, CoreState &b);
double getL3CacheHit(CoreState &a, CoreState &b);

void dump(CoreState &a, CoreState &b);
void getCoreCounterState(CoreState &state);
int32_t initpcm();

#endif
