//	File:	Worker.h
//	Author:	Tanner Broaddus

#ifndef WORKER_H
#define WORKER_H

namespace ScrybeIO {


class Device; // Forward Declaration

// class Worker
class Worker {
	
	public:

		Worker();

		~Worker();

		void handle_conns(Device& IODev);

		bool check_running() const;

		bool check_fail() const;
	
	private:

		int n_accept_fail;
		int n_accept_loop;
		int n_add_fail;
		int n_add_loop;
		bool F_running;
		bool F_fail;


}; // class Worker
} // namespace ScrybeIO

#endif
