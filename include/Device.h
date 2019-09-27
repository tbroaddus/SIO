//	File:		Device.h
//	Author:		Tanner Broaddus


#ifndef DEVICE_H
#define DEVICE_H

#include <thread>
#include <string>
#include <vector>
#include <memory>

#include "Options.h"
#include "Worker.h"


namespace ScrybeIO {

class Device {


	public:
		
		friend Worker;

		Device();

		Device(void(*_handle)(std::string request, int
					client_sock),
				const Options& IO_Options);

		Device(Device& Device_obj);
	
		~Device();
		
		int init(void(*_handle)(std::string request, int client_sock), const
				Options& IO_Options);
		
		int init(void(*_handle)(std::string request, int client_sock));

		int set_listen();

		int start();

		int stop();

		int pause();

		int resume();

		int reset();

		int reset(const Options& IO_Options);

		int n_threads_running() const;
	

	private:

		void (*handle)(std::string request, int client_sock);
		std::vector<std::shared_ptr<Worker>> Worker_vec;
		std::vector<std::thread> thread_vec;
		int epoll_fd;
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
		bool F_init;
		bool F_running;
		bool F_stop;
		bool F_pause;
		bool F_reset_callable;
		bool F_listening;


}; // Class Device
} // namespace ScrybeIO

#endif
