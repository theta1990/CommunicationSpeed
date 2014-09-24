/*
 * MyClock.h
 *
 *  Created on: Sep 22, 2014
 *      Author: volt
 */

#ifndef MYCLOCK_H_
#define MYCLOCK_H_

#include <sys/time.h>

class MyClock {

public:
	virtual ~MyClock();

	static MyClock* getInstance();

	void begin();
	void end();
	void stop();
	void resume();

	long getTimeBySec();
	long getTimeByMSec();
	long getTimeByUSec();

private:
	MyClock();
	static MyClock *m_ins;

	struct timeval m_begin;
	struct timeval m_end;

	long m_timeByUSec;
};


#endif /* MYCLOCK_H_ */
