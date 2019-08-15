//	File:	Device.cpp
//	Author:	Tanner Broaddus

#include "Device.h"

namespace ScrybeIO {



Device::Device(void(*_handle)(std::string request, int client_sock), const
		Options& IO_Options) {

	handle = _handle;
	port = IO_Options.port();
	thread_count = IO_Options.tc();
	buffer_size = IO_Options.buffer_size();
	accept_fail_limit = IO_Options.accept_fail_limit();
	accept_loop_reset = IO_Options.accept_loop_reset();
	add_fail_limit = IO_Options.add_fail_limit();
	add_loop_reset = IO_Options.add_loop_reset();
	max_events = IO_Options.max_events();
	max_listen = IO_Options.max_listen();
	timeout = IO_Options.timeout();
}



Device::~Device() {
	if (F_listening == true) 
		close(listen_sock);
	thread_vec.clear();
	Worker_vec.clear();
}



int Device::set_listen() {
	if (F_running == true) {
		cerr << "Device ERR in set_listen(): ";
		cerr << "Device already listening and running\n";
		return -1;
	}
	if (F_stop == true || F_pause == true) {
		cerr << "Device ERR in set_listen(): ";
		cerr << "Must call reset() before set_listen()\n";
		return -1;
	}
	F_reset_callable = false;
	listen_sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (listen_sock == -1) {
		cerr << "Device ERR in set_listen(): ";
		cerr << "Cannot create listening socket\n";
		F_reset_callable = true;
		return -1;
	}
	int set_sock = 1;
	// Allows bind() without error after reset() call
	if (setsockopt(listen_sock, SOL_Socket, SO_REUSEPORT, &set_sock,
				sizeof(set_sock)) == -1) {
		cerr << "Device ERR in set_listen(): ";
		cerr << "Failure in setsockopt()\n";
		close(listen_sock);
		F_reset_callable = true;
		return -1;
	}
	sockaddr_in listen_addr;
	listen_addr.sin_family = AF_INET;
	listen_addr.sin_port = htons(port);
	if (bind(listen_sock, (sockaddr*)&listen_addr, sizeof(listen_addr)) == -1)
	{
		cerr << "Device ERR in set_listen(): ";
		cerr << "Failure to bind listening socket\n";
		close(listen_sock);
		F_reset_callable = true;
		return -1;
	}
	F_listening = true;

	return 0;
} // set_listen()



int Device::start() {
	if (F_listening == false) {
		cerr << "Device ERR in start(): ";
		cerr << "Must call listening() before start()\n";
		return -1;
	}
	if (F_running == true) {
		cerr << "Device ERR in start(): Device already running\n";
		return -1;
	}
	if (F_pause == true) {
		cerr << "Device ERR in start(): ";
		cerr << "Must call reset(), set_listen() before start()\n";
		return -1;
	}
	if (thread_count < 1) {
		cerr << "Device ERR in start(): ";
		cerr << "thread_count must be at least 1\n";
		close(listen_sock);
		F_reset_callable = true;
		return -1;
	}
	F_running = true;
	F_stop = false;

	for(int i = 0; i < thread_count; i++) {
		Worker_vec.push_back(std::make_shared<Worker>(listen_sock));
	}

	for(std::shared_ptr<Worker> Worker_ptr : Worker_vec) {
		thread_vec.push_back(std::move(std::thread(&Worker::handle_conns,
						Worker_ptr, std::ref(*this))));
	}

	return 0;
} // start()



int Device::stop() {
	(F_running == false) {
		cerr << "Device ERR in stop(): stop() called while not running\n";
		return -1;
	}
	F_stop = true;
	for(std::thread& Worker_thread : thread_vec) {
		Worker_thread.join();
	}
	thread_vec.clear();
	Worker_vec.clear();
	close(listen_sock);
	F_listening = false;
	F_running = false;
	F_reset_callable = true;

	return 0;
} // stop()



int Device::pause() {
	if (F_running == false) {
		cerr << "Device ERR in pause(): ";
		cerr << "pause() called while device not running\n";
		return -1;
	}
	F_pause = true;
	F_running = false;
	for(std::thread& Worker_thread : thread_vec) {
		Worker_thread.join();
	}
	thread_vec.clear();
	F_reset_callable = true;
	int n_Workers_failed = 0;
	for(std::shared_ptr<Worker> Worker_ptr : Worker_vec) {
		if (Worker_ptr->check_fail()) 
			n_Workers_failed++;
	}
	if (n_Workers_failed > 0) {
		cerr << "Device ERR in pause(): ";
		cerr << n_Workers_failed << " Worker threads failed\n";
		close(listen_sock);
		F_listening = false;
		F_pause = false; // We do not want the ability to call resume() if all
						 // Worker threads failed
		return -1;	
	}
	
	return 0;
} // pause() 



int Device::resume() {
	if (F_pause == false) {
		cerr << "Device ERR in resume(): ";
		cerr << "Device was could not be resumed\n";
		return -1;
	}
	F_reset_callable = false;
	F_pause = false;
	for(std::shared_ptr<Worker> Worker_ptr : Worker_vec) {
		thread_vec.push_back(std::move(std::thread(&Worker::handle_conns,
						Worker_ptr, std::ref(*this))));
	}
	F_running = true;
	
	return 0;
} // resume() 



int Device::reset() {
	if (F_reset_callable == false) {
		cerr << "Device ERR in reset(): reset() not callable\n";
		return -1;
	}
	if (F_listening == true) {
		close(listen_sock);
		F_listening = false;
	}
	Worker_vec.clear();
	F_stop = false;
	F_pause = false;
	F_master_fail = false;

	return 0;
} // reset() 



int Device::reset(const Options& IO_Options) {
	if (F_reset_callable == false) {
		cerr << "Device ERR in reset(): reset() not callable\n";
		return -1;
	}
	port = IO_Options.port();
	thread_count = IO_Options.tc();
	buffer_size = IO_Options.buffer_size();
	accept_fail_limit = IO_Options.accept_fail_limit();
	accept_loop_reset = IO_Options.accept_loop_reset();
	add_fail_limit = IO_Options.add_fail_limit();
	add_loop_reset = IO_Options.add_loop_reset();
	max_events = IO_Options.max_events();
	max_listen = IO_Options.max_listen();
	timeout = IO_Options.timeout();
	if (F_listening == true) {
		close(listen_sock);
		F_listening = false;
	}
	Worker_vec.clear();
	F_stop = false;
	F_pause = false;
	F_master_fail = false;

	return 0;
} // reset(const Options&)



int Device::n_threads_running() const {
	if (F_running == false)
		return 0;
	int n_running = 0;
	for (std::shared_ptr<Worker> Worker_ptr : Worker_vec) {
		if (Worker_ptr->check_running())
			n_running++;
	}
	
	return n_running;
} // n_running() 


} // namespace ScrybeIO
