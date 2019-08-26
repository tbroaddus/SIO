//	File:		Options.h
//	Author:		Tanner Broaddus

#ifndef OPTIONS_H
#define OPTIONS_H

namespace ScrybeIO {

class Options {

	
	public:

		
		Options();

		~Options();

		void set_port(const int _port);
		void set_tc(const int _tc);
		void set_buffer_size(const int _buffer_size);
		void set_accept_fail_limit(const int _accept_fail_limit);
		void set_accept_loop_reset(const int _accept_loop_reset);
		void set_add_fail_limit(const int _add_fail_limit);
		void set_add_loop_reset(const int _add_loop_reset);
		void set_max_events(const int _max_events);
		void set_max_listen(const int _max_listen);
		void set_timeout(const int _timeout);

		int get_port() const;
		int get_tc() const;
		int get_buffer_size() const;
		int get_accept_fail_limit() const;
		int get_accept_loop_reset() const;
		int get_add_fail_limit() const;
		int get_add_loop_reset() const;
		int get_max_events() const;
		int get_max_listen() const;
		int get_timeout() const;



	private:

		int port;
		int tc;
		int buffer_size;			
		int accept_fail_limit;
		int accept_loop_reset;
		int add_fail_limit;
		int add_loop_reset;
		int max_events;
		int max_listen;
		int timeout;			// In milliseconds

}; // Class Options
} // namespace ScrybeIO

#endif

