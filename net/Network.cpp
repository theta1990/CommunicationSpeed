/*
 * Network.cpp
 *
 *  Created on: Sep 22, 2014
 *      Author: volt
 */
#include <malloc.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstdio>
#include <pthread.h>
#include <string.h>
#include <iostream>
#include <getopt.h>
#include "../clc/MyClock.h"

#define MAXCONN 5
#define TESTCASE 100000

void *receiveHandler(void *para) {

	int &serverSockFd = *(int *) para;
	int clientSockFd;
	sockaddr_in addr;
	unsigned int size;
	clientSockFd = accept(serverSockFd, (sockaddr *) &addr, &size);

	while (true) {
		char *buf = new char[128];
		int nread = 128;
		read(clientSockFd, buf, nread);

		int nwrite;
		nwrite = strlen(buf);
		write(clientSockFd, buf, nwrite);
	}
	return 0;
}

int serverThread() {

	int serverSockFd;
	sockaddr_in serverSocket;

	serverSocket.sin_family = AF_INET;
	serverSocket.sin_port = htons(8001);
	serverSocket.sin_addr.s_addr = inet_addr("10.11.1.235");

	if ((serverSockFd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		printf("socket error");
		return -1;
	}

	/* Enable address reuse */
	int on = 1;
	setsockopt(serverSockFd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	if (bind(serverSockFd, (sockaddr *) &serverSocket, sizeof(serverSocket))
			< 0) {
		perror("Server:: bind error!");
		return -1;
	}

	int backLog = MAXCONN - 1;
	if (listen(serverSockFd, backLog) == -1) {
		printf("Server::listen error!\n");
		return -1;
	} else {
		printf("Server::listen ok!\n");
	}

	pthread_t t_Receiver;

	const int error = pthread_create(&t_Receiver, NULL, receiveHandler,
			&serverSockFd);
	if (error != 0) {
		std::cout << "cannot create receive thread!" << std::endl;
	}

	printf("Receiver thread id=%x\n", t_Receiver);

	void *result;
	pthread_join(t_Receiver, &result);

	return 0;
}

int clientThread(char *host) {

	int clientSockFd;

	sockaddr_in serverSockAddr;

	clientSockFd = socket(PF_INET, SOCK_STREAM, 0);

	memset(&serverSockAddr, 0, sizeof(serverSockAddr));
	serverSockAddr.sin_family = AF_INET;
	serverSockAddr.sin_addr.s_addr = inet_addr("10.11.1.235");
	serverSockAddr.sin_port = htons(8001);

	int result = connect(clientSockFd, (struct sockaddr *) &serverSockAddr,
			sizeof(serverSockAddr));

	if (result < 0) {
		perror("Client:: error on connecting \n");
	} else {
		printf("Client::succed in connection\n");
	}

	char *buf = new char[64];
	strcpy(buf, "hello world");

	MyClock::getInstance()->begin();
	MyClock::getInstance()->stop();

	for (int i = 0; i < TESTCASE; ++i) {
		MyClock::getInstance()->resume();
		write(clientSockFd, buf, strlen(buf));
		read(clientSockFd, buf, strlen(buf));
		MyClock::getInstance()->stop();
		if( strcmp(buf, "hello world") != 0 ) perror("recv error");
	}

	close(clientSockFd);
	printf("Time cost: %ld\n", MyClock::getInstance()->getTimeByUSec());
	printf("Time cost percase: %ld\n", MyClock::getInstance()->getTimeByUSec() / TESTCASE);
	return 0;
}

int main(int argc, char **argv){

	int c;
	c = getopt(argc, argv, "s:");
	
	if ( c != -1 ){
		clientThread(optarg);
	}else {
		serverThread();
	}
		
	return 0;
}
