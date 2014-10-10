/*
 * Generate.cpp
 *
 *  Created on: Oct 10, 2014
 *      Author: volt
 */

#include "Generate.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <assert.h>
using namespace expdb;
namespace expdb {

Generate::Generate() {
	// TODO Auto-generated constructor stub

	posix_memalign((void **) &m_buf, 512, m_SIZE);
}

Generate::~Generate() {
	// TODO Auto-generated destructor stub
	delete m_buf;
}

char* Generate::genRecord() {

	for (int i = 0; i < m_SIZE; ++i) {
		m_buf[i] = rand() % 255;
	}
	return m_buf;
}

const char* Generate::getBuf() {

	return m_buf;
}

const int Generate::getSize() {

	return m_SIZE;
}

} /* namespace expdb */
int main(int argc, char **argv) {

	int fileSize = 20;
	int bufferSize;

	int opt;
	while ((opt = getopt(argc, argv, "f:s:")) >= 0) {

		switch (opt) {
		case 'f':
			fileSize = strtol(optarg, NULL, 10);
			break;
		case 's':
			bufferSize = strtol(optarg, NULL, 10);
			break;
		default:
			printf("-f file size\n");
			printf("-s buffer size\n");
			exit(0);
		}
	}

	Generate gen;
	struct timeval begin, end;
	int wcode;

//	printf("-fileSize: %d\n-bufferSize: %d\n", fileSize, gen.getSize());

	int fd = open("raw.txt", O_DIRECT | O_WRONLY | O_CREAT, 00700);

	if (fd < 0) {
		perror("Open error: ");
		exit(0);
	}

	const char *text = gen.genRecord();

	gettimeofday(&begin, NULL);

	for (int i = 0; i < fileSize; ++i) {

		wcode = write(fd, text, gen.getSize());
		if (wcode < 0){
			perror("-Write error: ");
			fflush(stdout);
			break;
		}
		else
			assert(wcode == gen.getSize());

	}

	gettimeofday(&end, NULL);

	fprintf(stdout,
			"-Time consumed to generate %d * 4096 bytes: %ld\n", fileSize,
			(end.tv_sec - begin.tv_sec));
	fprintf(stdout, "-Write performance: %lf MB/s\n",
			(1.0 * fileSize * gen.getSize())
					/ (1024 * 1024 * (end.tv_sec - begin.tv_sec)));

	close(fd);
}
