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



Device::~Device() {}



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
	close(listen_sock);
	Worker_vec.clear();
	thread_vec.clear();
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
	for(std::shared_ptr<Worker> Worker_ptr : Worker_Vec) {
		if (Worker_ptr->check_fail()) 
			n_Workers_failed++;
	}
	if (n_Workers_failed > 0) {
		cerr << "Device ERR in pause(): ";
		cerr << n_Workers_failed << " Worker threads failed\n"
		F_pause = false; // We do not want the ability to call resume() if all
						 // Worker threads failed
		return -1;	
	}
	
	return 0;
} // pause() 



int Device::resume() {

} // resume() 



int Device::reset() {

} // reset() 



int Device::reset(const int _port) {

} // reset(int)



int Device::reset(const Options& IO_Options) {

} // reset(const Options&)



int Device::n_running() const {

} // n_running() 



bool Device::check_master_fail() const {

} // check_master_fail()



void Device::set_master_fail() {

} // set_master_fail() 



} // namespace ScrybeIO
