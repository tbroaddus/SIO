//	File:		Options.cpp
//	Author:		Tanner Broaddus

#include "Options.h"

using namespace ScrybeIO;

	Options::Options(){}

	Options::~Options(){}

	
	// Setters
	void Options::set_port(const int _port) {
		port = _port;	
	}

	void Options::set_tc(const int _tc) {
		tc = _tc;
	}

	void Options::set_buffer_size(const int _buffer_size) {
		buffer_size = _buffer_size;
	}

	void Options::set_accept_fail_limit(const int _accept_fail_limit) {
		accept_fail_limit = _accept_fail_limit;
	}

	void Options::set_accept_loop_reset(const int _accept_loop_reset) {
		accept_loop_reset = _accept_loop_reset;
	}

	void Options::set_add_fail_limit(const int _add_fail_limit) {
		add_fail_limit = _add_fail_limit;
	}

	void Options::set_add_loop_reset(const int _add_loop_reset) {
		add_loop_reset = _add_loop_reset;
	}

	void Options::set_max_events(const int _max_events) {
		max_events = _max_events;
	}

	void Options::set_max_listen(const int _max_listen) {
		max_listen = _max_listen; 
	}

	void Options::set_timeout(const int _timeout) {
		timeout = _timeout;
	}


	// Getters
	int Options::get_port() const {
		return port;
	}

	int Options::get_tc() const {
		return tc;
	}

	int Options::get_buffer_size() const {
		return buffer_size;
	}

	int Options::get_accept_fail_limit() const {
		return accept_fail_limit;
	}

	int Options::get_accept_loop_reset() const {
		return accept_loop_reset;
	}

	int Options::get_add_fail_limit() const {
		return add_fail_limit;
	}

	int Options::get_add_loop_reset() const {
		return add_loop_reset;
	}

	int Options::get_max_events() const {
		return max_events;
	}

	int Options::get_max_listen() const {
		return max_listen;
	}

	int Options::get_timeout() const {
		return timeout;
	}

