/*
 * Generate.h
 *
 *  Created on: Oct 10, 2014
 *      Author: volt
 */

#ifndef GENERATE_H_
#define GENERATE_H_

#include <stddef.h>

namespace expdb {

class Generate {
public:
	Generate();
	virtual ~Generate();

	char* genRecord();

	const char* getBuf();

	const int getSize();

private:
	const static int m_SIZE = 4096;
//	const static int m_defaultSize = 4096;
	char *m_buf;
};

} /* namespace expdb */

#endif /* GENERATE_H_ */
