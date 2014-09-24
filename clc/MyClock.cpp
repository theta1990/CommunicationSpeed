/*
 * MyClock.cpp
 *
 *  Created on: Sep 22, 2014
 *      Author: volt
 */

#include "MyClock.h"
#include <sys/time.h>
#include <stdlib.h>


MyClock* MyClock::m_ins = 0;

MyClock::MyClock() {
	// TODO Auto-generated constructor stub
	m_timeByUSec = 0;
}

MyClock::~MyClock() {
	// TODO Auto-generated destructor stub
}

MyClock * MyClock::getInstance(){

	if( m_ins == 0 ) m_ins = new MyClock();
	return m_ins;
}

void MyClock::begin() {

	gettimeofday(&m_begin, NULL);
	m_timeByUSec = 0;
}

void MyClock::stop() {

	gettimeofday(&m_end, NULL);
	m_timeByUSec += 1000000 * (m_end.tv_sec - m_begin.tv_sec) + m_end.tv_usec
			- m_begin.tv_usec;

}

void MyClock::resume() {

	gettimeofday(&m_begin, NULL);
}

void MyClock::end() {

	gettimeofday(&m_end, NULL);
	m_timeByUSec += 1000000 * (m_end.tv_sec - m_begin.tv_sec) + m_end.tv_usec
			- m_begin.tv_usec;
}

long MyClock::getTimeBySec(){

	return m_timeByUSec / 1000000;
}

long MyClock::getTimeByMSec(){

	return m_timeByUSec / 1000;
}

long MyClock::getTimeByUSec(){

	return m_timeByUSec;
}
