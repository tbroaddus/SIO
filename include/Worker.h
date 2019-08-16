//	File:	Worker.h
//	Author:	Tanner Broaddus

#ifndef WORKER_H
#define WORKER_H

#include <iostream>
#include <cstdio>

#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <netdb.h>
#include <errno.h>

using std::cout;
using std::endl;
using std::cerr;


namespace ScrybeIO {

class Device; // Forward Declaration

class Worker {
	
	public:

		Worker();

		~Worker();

		void handle_conns(Device& IODev);

		bool check_running() const;

		bool check_fail() const;
	
	private:

		int n_accept_fail = 0;
		int n_accept_loop = 0;
		int n_add_fail = 0;
		int n_add_loop = 0;
		bool F_running = false;
		bool F_fail = false;



}; // class Worker
} // namespace ScrybeIO

#endif
