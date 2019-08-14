//	File:		Device.h
//	Author:		Tanner Broaddus

#ifndef DEVICE_H
#define DEVICE_H

#include <iostream>
#include <cstdio>
#include <thread>
#include <string>
#include <vector>
#include <memory>

#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <netdb.h>
#include <errno.h>

#include "Options.h"
#include "Worker.h"

using std::cout;
using std::endl;
using std::cerr;


namespace ScrybeIO {

class Device {


	public:
		
		friend Worker;

		Device(void(*_handle)(std::string request, int
					client_sock),
				const Options& IO_Options);
	
		~Device();

		int set_listen();

		int start();

		int stop();

		int pause();

		int resume();

		int reset();

		int reset(const int _port);

		int reset(Options& IO_Options);

		int n_running() const;

		bool check_master_fail() const;

		void set_master_fail();
		

	private:

		void (*handle)(std::string request, int client_sock);
		std::vector<std::shared_ptr<Worker>> Worker_vec;
		std::vector<std::thread> thread_vec;
		int listen_sock;
		int port;
		int thread_count;
		int buffer_size;
		int accept_fail_limit;
		int accept_loop_reset;
		int add_fail_limit;
		int add_loop_reset;
		int max_events;
		int max_listen;
		int timeout;
		bool F_running = false;
		bool F_stop = false;
		bool F_pause = false;
		bool F_master_fail = false;
		bool F_reset_callable = false;
		bool F_listening = false;



}; // Class Device
} // namespace ScrybeIO

#endif
