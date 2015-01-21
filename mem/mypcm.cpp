#include "mypcm.h"
#include <stdio.h>

double getL1CacheHit(CoreState &a, CoreState &b) {

	return 1.0 * (b.Event1 - a.Event1) / (b.Event0 - a.Event0);
}

double getL2CacheHit(CoreState &a, CoreState &b) {

	return 1.0 * (b.Event2 - a.Event2) / (b.Event0 - a.Event0);
}

double getL3CacheHit(CoreState &a, CoreState &b) {

	return 1.0 * (b.Event3 - a.Event3) / (b.Event0 - a.Event0);
}

void getCoreCounterState(CoreState &state) {

	*static_cast<CoreCounterState *>(&state) = getCoreCounterState(0);
}

void dump(CoreState &a, CoreState &b){

	printf("L1 hit count: %d\n", b.Event1 - a.Event1);
	printf("L2 hit count: %d\n", b.Event2 - a.Event2);
	printf("L3 hit count: %d\n", b.Event3 - a.Event3);
	printf("Load count: %d\n", b.Event0 - a.Event0);
}

int32_t initpcm() {

	int ret = -1;
	PCM *m = PCM::getInstance();

	PCM::CustomCoreEventDescription para[4];

	if (m != NULL) {
		para[0].event_number = MEM_UOP_RETIRED;
		para[0].umask_value = LOADS;

		para[1].event_number = MEM_LOAD_UOPS_RETIRED_EVENT;
		para[1].umask_value = L1_HIT;

		para[2].event_number = MEM_LOAD_UOPS_RETIRED_EVENT;
		para[2].umask_value = L2_HIT;

		para[3].event_number = MEM_LOAD_UOPS_RETIRED_EVENT;
		para[3].umask_value = LLC_HIT;

		ret = m->program(PCM::CUSTOM_CORE_EVENTS, para);
	}
	return ret;
}

//TEST(pcmtest);
