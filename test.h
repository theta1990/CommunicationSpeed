/*
 * test.h
 *
 *  Created on: Jan 9, 2015
 *      Author: volt
 */

#ifndef TEST_H_
#define TEST_H_
#include "debuglog.h"
typedef int (*testfunc)(int , char **);

extern testfunc func;
#define TEST(arg) testfunc func = arg


#endif /* TEST_H_ */
