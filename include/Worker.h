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

#include "Device.h"
#include "Options.h"

using std::cout;
using std::endl;
using std::cerr;


namespace ScrybeIO {

class Worker {
	
	public:

		Worker();

		~Worker();

		void handle_conns(Device& IODev);

		bool check_running() const;

		bool check_fail() const;

		bool ready() const;
	
	private:

		int epoll_fd;
		int n_accept_fail = 0;
		int n_accept_loop = 0;
		int n_add_fail = 0;
		int n_add_loop = 0;
		bool F_running = false;
		bool F_ready = false;
		bool F_fail = false;



}; // class Worker
} // namespace ScrybeIO

#endif
