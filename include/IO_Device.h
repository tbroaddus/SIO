//	File:		IO_Device.h
//	Author:		Tanner Broaddus
/*
NOTES:


*/

#ifndef IO_DEVICE_H
#define IO_DEVICE_H


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

using std::cout;
using std::endl;
using std::cerr;


namespace ScrybeIO {
namespace IODevice {




// struct Options
// =========================================================

struct Options {

	Options(){}

	Options(int _port, int _tc, int _bs, int _accept_fl, int _accept_lr, int _add_fl,
			int _add_lr, int _max_events, int _max_listen, int _at, int _ht)
	{
		port = _port;
		thread_count = _tc;
		buffer_size = _bs;
		accept_fail_limit = _accept_fl;
		accept_loop_reset = _accept_lr;
		add_fail_limit = _add_fl;
		add_loop_reset = _add_lr;
		max_events = _max_events;
		max_listen = _max_listen;
		accept_timeout = _at;
		handle_timeout = _ht;
	}

	int port;
	int thread_count = 2;
	int buffer_size = 4096;
	int accept_fail_limit = 1;
	int accept_loop_reset = 10;
	int add_fail_limit = 1;
	int add_loop_reset = 10;
	int max_events = 50;
	int max_listen = 100;
	int accept_timeout = 1000;		// In milliseconds
	int handle_timeout = 1000;		// In milliseconds
};




// class Device
// =========================================================

class Device {


//	--- Public ---

	public:


		// Constructor
		explicit Device(void(*_handle)(std::string request, int client_sock),
				const struct Options& opt)
		{
			handle = _handle;
			port = opt.port;
			thread_count = opt.thread_count;
			buffer_size = opt.buffer_size;
			accept_fail_limit = opt.accept_fail_limit;
			accept_loop_reset = opt.accept_loop_reset;
			add_fail_limit = opt.add_fail_limit;
			add_loop_reset = opt.add_loop_reset;
			max_events = opt.max_events;
			max_listen = opt.max_listen;
			accept_timeout = opt.accept_timeout;
			handle_timeout = opt.handle_timeout;
		}


		// Destructor
		~Device() {
			close(listen_sock);
		}



		// Acceptor and handler now have access to Device's private members
		friend struct acceptor;
		friend struct handler;




		// --- Public User Functions ---
		// - set_listen()
		// - start()
		// - stop()
		// - pause()
		// - resume()
		// - reset()
		// - n_running() 
		// - check_master_fail()


		// set_listen()
		const int set_listen() {
			if (F_running == true) {
				cerr << "Device ERR in set_listen(): IO device already listening and running\n";
				return -1;
			}
			if (F_stop == true || F_pause == true) {
				cerr << "Device ERR in set_listen(): Must call reset() before set_listen()\n";
				return -1;
			}
			F_reset_callable = false;
			listen_sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
			if (listen_sock == -1) {
				cerr << "Device ERR in set_listen(): Cannot create listening socket\n";
				return -1;
			}

			int set_sock = 1;
			// Allows bind() without error after reset() call
			if (setsockopt(listen_sock, SOL_SOCKET, SO_REUSEPORT, &set_sock,
					sizeof(set_sock)) == -1) {
				cerr << "Device ERR in set_listen(): Failure in setsockopt() in IO_Device\n";
				close(listen_sock);
				F_reset_callable = true;
				return -1;
			}

			sockaddr_in listen_addr;
			listen_addr.sin_family = AF_INET;
			listen_addr.sin_port = htons(port);
			inet_pton(AF_INET, "0.0.0.0", &listen_addr.sin_addr);
			if (bind(listen_sock, (sockaddr*)&listen_addr, sizeof(listen_addr))
					== -1) {
					cerr << "Device ERR in set_listen(): Failure to bind listening sock\n";
					close(listen_sock);
					F_reset_callable = true;
					return -1;
			}
			if (listen(listen_sock, max_listen) == -1) {
				cerr << "Device ERR in set_listen(): Failed to listen\n";
				F_reset_callable = true;
				close(listen_sock);
				return -1;
			}

			F_listening = true;
			return 0;
		} // set_listen()




		// start()
		const int start() {
			if (F_listening == false) {
				cerr << "Device ERR in start(): Must call listening() before calling start()\n";
				return -1;
			}
			if (F_running == true) {
				cerr << "Device ERR in start(): IO Device already running\n";
				return -1;
			}
			if (F_pause == true) {
				cerr << "Device ERR in start(): Must call reset(), set_listen() before start()\n";
				return -1;
			}
			if (thread_count < 2) {
				cerr << "Device ERR in start(): thread_count must be greater than 1\n";
				close(listen_sock);
				F_reset_callable = true;
				return -1;
			}
			F_running = true;
			F_stop = false;
			acc_epoll_fd = epoll_create1(0);
			han_epoll_fd = epoll_create1(0);
			if (acc_epoll_fd == -1 || han_epoll_fd == -1) {
				cerr << "Device ERR in start(): Failure in epoll_create1(0)\n";
				close(listen_sock);
				F_listening = false;
				F_running = false;
				F_reset_callable = true;
				return -1;
			}

			struct epoll_event event;
			event.events = EPOLLIN | EPOLLET;
			event.data.fd = listen_sock;
			if (epoll_ctl(acc_epoll_fd, EPOLL_CTL_ADD, listen_sock, &event) == -1)
				{
					cerr << "Device ERR in start(): Failed to call epoll_ctl() in start()\n";
					close(listen_sock);
					F_listening = false;
					F_running = false;
					F_reset_callable = true;
					return -1;
			}

			for(int i = 0; i < thread_count-1; i++) {
				handler_vec.push_back(std::make_shared<handler>());
			}

			std::thread L1(&acceptor::accept_conns, &acceptor_L1, std::ref(*this));
			thread_vec.push_back(std::move(L1));

			for(std::shared_ptr<handler> handler_ptr : handler_vec) {
					thread_vec.push_back(std::move(std::thread(&handler::handle_conns,
									handler_ptr, std::ref(*this))));
			}

			return 0;
		} // start()




		// stop()
		const int stop() {
			if (F_running != true) {
				cerr << "Device ERR in stop(): stop() called while not running\n";
				return -1;
			}
			F_stop = true;
			for(std::thread& t : thread_vec) {
				t.join();
			}
			thread_vec.clear();
			F_running = false;
			F_reset_callable = true;
			if (F_master_fail == true) {
				return -1;
				cerr << "Device ERR in stop(): Master fail detected in stop()\n";

			}

			return 0;
		} // stop()




		// pause()
		const int pause() {
			if (F_running != true) {
				cerr << "Device ERR in pause(): pause() called while device not running\n";
				return -1;
			}
			F_pause = true;
			F_running = false;
			for(std::thread& t : thread_vec) {
				t.join();
			}
			thread_vec.clear();
			F_reset_callable = true;
			if (F_master_fail == true) {
				cerr << "Device ERR in pause(): Master fail detected in pause()\n";
				return -1;
			}

			return 0;
		} // pause()




		// resume()
		const int resume() {
			if (F_pause != true)  {
				cerr << "Device ERR in resume(): Device was not paused when resume() was called\n";
				return -1;
			}

			F_reset_callable = false;
			F_pause = false;

			std::thread L1(&acceptor::accept_conns, &acceptor_L1,
					std::ref(*this));
			thread_vec.push_back(std::move(L1));
			for(std::shared_ptr<handler> handler_ptr : handler_vec) {
				thread_vec.push_back(std::move(std::thread(&handler::handle_conns,
								handler_ptr, std::ref(*this))));
			}

			F_running = true;

			return 0;
		} // resume()



		// reset() w/o params
		const int reset() {
			if (F_reset_callable == false) {
				cerr << "Device ERR in reset(): reset() not callable\n";
				return -1;
			}
			handler_vec.clear();
			struct acceptor _acceptor;
			acceptor_L1 = _acceptor;
			F_listening = false;
			F_running = false;
			F_stop = false;
			F_master_fail = false;
			F_pause = false;

			return 0;
		} // reset() w/o params



		// reset() with port
		const int reset(const int _port) {
			if (F_reset_callable == false) {
				cerr << "Device ERR in reset(): reset() not callable\n";
				return -1;
			}
			port = _port;
			handler_vec.clear();
			struct acceptor _acceptor;
			acceptor_L1 = _acceptor;
			F_listening = false;
			F_running = false;
			F_stop = false;
			F_master_fail = false;
			F_pause = false;

			return 0;
		} // reset() with port



		// reset() w/ options
		const int reset(const struct Options& opt) {
			if (F_reset_callable == false) {
				cerr << "Device ERR in reset(): reset() not callable\n";
				return -1;
			}
			port = opt.port;
			thread_count = opt.thread_count;
			buffer_size = opt.buffer_size;
			accept_fail_limit = opt.accept_fail_limit;
			accept_loop_reset = opt.accept_loop_reset;
			add_fail_limit = opt.add_fail_limit;
			add_loop_reset = opt.add_loop_reset;
			max_events = opt.max_events;
			max_listen = opt.max_listen;
			accept_timeout = opt.accept_timeout;
			handle_timeout = opt.handle_timeout;
			handler_vec.clear();
			struct acceptor _acceptor;
			acceptor_L1 = _acceptor;
			F_listening = false;
			F_running = false;
			F_stop = false;
			F_master_fail = false;
			F_pause = false;

			return 0;
		}




		// n_running()
		const int n_running() const {
			if (F_master_fail == true)
				return 0;
			if (F_running == false)
				return 0;
			int thread_count = 1; // Acceptor running
			for (std::shared_ptr<handler> handler_ptr : handler_vec) {
				if (handler_ptr->F_han_fail == false)
					thread_count++;
			}
			return thread_count;
		} // n_running()




		// check_master_fail()
		const bool check_master_fail() const {
			return F_master_fail;
		} // check_master_fail()



		// IMPORTANT! For testing purposes only, will not make it in final
		// release.
		// set_master_fail();
		void set_master_fail() {
			F_master_fail = true;
		}



		
//	--- Private ---	

	private:


		// struct acceptor
		struct acceptor {

			void accept_conns(Device& IODev) {
				struct epoll_event event, events[1]; // One because we are only wanting
			   		                              // to observe the listening
												  // socket.
				// Main event loop
				while(true) {
					// cout << "Acceptor loop executing" << endl;
					if (IODev.F_master_fail == true || IODev.F_stop == true) {
						close(IODev.listen_sock);
						IODev.F_listening = false;
						break;
					}
					if (F_acc_fail == true) {
						IODev.F_master_fail = true;
						close(IODev.listen_sock);
						IODev.F_listening = false;
						break;
					}
					if (IODev.F_pause == true) {
						break;
					}
					// Checking to see if handler threads are still
					// functioning.
					int han_fail_count = 0;
					for (std::shared_ptr<handler> handler_ptr : IODev.handler_vec) {
						if (handler_ptr->F_han_fail == true)
							han_fail_count++;
					}
					if (han_fail_count == IODev.thread_count - 1) {
						IODev.F_master_fail = true;
						continue;
					}
					int nfds = epoll_wait(IODev.acc_epoll_fd, events, 1, IODev.accept_timeout);
					if (nfds == -1) {
						cerr << "acceptor ERR in accept_conns(): Failure in epoll_wait() in acceptor\n";
						F_acc_fail = true;
						continue;
					}
					if (nfds > 0) {
						// Accept loop
						while (true) {
							if (n_accept_loop == IODev.accept_loop_reset) {
								n_accept_fail = 0;
								n_accept_loop = 0;
							}
							if (n_add_loop == IODev.add_loop_reset) {
								n_add_fail = 0;
								n_add_loop = 0;
							}
							struct sockaddr_in client_addr;
							socklen_t client_size = sizeof(client_addr);
							int client_sock = accept(IODev.listen_sock,
									(sockaddr*)&client_addr, &client_size);
							if (client_sock == -1) {
								if ((errno == EAGAIN || errno == EWOULDBLOCK))
									break; // Added all new connections
								else {
									cerr << "acceptor ERR in accept_conns(): Failed to accept new client\n";
									n_accept_fail++;
									n_accept_loop++;
									if (n_accept_fail == IODev.accept_fail_limit) {
										F_acc_fail = true;
									}
									break; // could not accept;
								}
							}
							int flags = fcntl(client_sock, F_GETFL, 0);
							flags |= O_NONBLOCK;
							fcntl(client_sock, F_SETFL, flags);
							event.events = EPOLLIN | EPOLLET;
							event.data.fd = client_sock;
							if (epoll_ctl(IODev.han_epoll_fd, EPOLL_CTL_ADD, client_sock,
										&event) == -1) {
								cerr << "acceptor ERR in accept_conns(): Failure in epoll_ctl to add fd\n";
								close(client_sock);
								n_add_fail++;
								n_add_loop++;
								if (n_add_fail == IODev.add_fail_limit) {
									F_acc_fail = true;
								}
								break;
							}
							n_accept_loop++;
							n_add_loop++;

							// ----- Testing purposes ----------------------

							char host[NI_MAXHOST];
							char svc[NI_MAXSERV];
							memset(host, 0, NI_MAXHOST);
							memset(svc, 0, NI_MAXSERV);
							int result = getnameinfo((sockaddr*)&client_addr,
									client_size, host, NI_MAXHOST, svc,
									NI_MAXSERV, 0);

							if (result) {
								cout << host << " connected on " << svc <<
									endl;
							}
							else {
								inet_ntop(AF_INET, &client_addr.sin_addr, host,
										NI_MAXHOST);
								cout << host << " connected on " <<
									ntohs(client_addr.sin_port) << endl;
							}

							// ----------------------------------------------


						} // while()
					} // if ()
				} // while()
			} // accept_conns();

			//	struct acceptor public member variables
			int n_accept_fail = 0;
			int n_accept_loop = 0;
			int n_add_fail = 0;
			int n_add_loop = 0;
			bool F_acc_fail = false;

		}; // struct acceptor





		// Handlers should NOT invoke a master fail. That is up to the
		// acceptor to do so.

		// struct handler
		struct handler {

			void handle_conns(Device& IODev) {
				struct epoll_event events[IODev.max_events];
				// main event loop
				while(true) {
					// cout << "Handler loop executing" << endl;
					if (F_han_fail == true || IODev.F_master_fail == true ||
							IODev.F_stop == true || IODev.F_pause == true)
						break;
					int nfds = epoll_wait(IODev.han_epoll_fd, events,
							IODev.max_events, IODev.handle_timeout);
					if (nfds == -1) {
						cerr << "handler ERR in handle_conns(): Failure in epoll_wait() in handler thread\n";
						F_han_fail = true;
						continue;
					}
					for(int i = 0; i < nfds; i++) {
						if ((events[i].events & EPOLLERR) || (events[i].events &
										EPOLLHUP)) {
							cerr << "handler ERR in handle_conns(): EPOLLERR in handler\n";
							if (epoll_ctl(IODev.han_epoll_fd, EPOLL_CTL_DEL,
										events[i].data.fd, NULL) == -1) {
								cerr << "handler ERR in handle_conns(): Could not delete client socket from epoll\n";
								F_han_fail = true;
								break;
							}
							close(events[i].data.fd);
							continue;
						}
						bool close_fd = false;
						char buff[IODev.buffer_size];
						memset(buff, 0, IODev.buffer_size);

						while (true) {
							int bytes_rec = recv(events[i].data.fd, buff,
									IODev.buffer_size, 0);
							if (bytes_rec < 0) {
								if (errno!= EWOULDBLOCK && errno != EAGAIN) {
									// cout << errno << endl;
									cerr << "handler ERR in handle_conns(): Could not receive message\n";
									close_fd = true;
								}
								// cout << "No more data to recv" << endl;
								break;
							}
							else if (bytes_rec == 0) {
								cout << "Connection closed" << endl;
								close_fd = true;
								break;
							}
							if (bytes_rec > 0) {
								// cout << bytes_rec << " bytes received" << endl;
								// handle_accept()
								int client_sock = events[i].data.fd;
								(*IODev.handle)(std::move(std::string(buff)),
										client_sock);
								continue;
								}
						}
						if (close_fd) {
							if (epoll_ctl(IODev.han_epoll_fd, EPOLL_CTL_DEL,
										events[i].data.fd, NULL) == -1) {
								cerr << "handler ERR in handle_conns(): Could not delete client socket from epoll\n";
								F_han_fail = true;
							}
							close(events[i].data.fd);
						}
					} // for()
				} // while()
				if (F_han_fail == true)
					cerr << "handler ERR in handle_conns(): handler thread failed\n";
			}

			// Struct handler public member variable
			bool F_han_fail = false;

		}; // struct handler

		


		// Private member variables
		void (*handle)(std::string request, int client_sock);
		std::vector<std::shared_ptr<handler>> handler_vec;
		std::vector<std::thread> thread_vec;
		acceptor acceptor_L1;
		int listen_sock;
		int acc_epoll_fd;
		int han_epoll_fd;
		int port;
		int thread_count;
		int buffer_size;
		int accept_fail_limit;
		int accept_loop_reset;
		int add_fail_limit;
		int add_loop_reset;
		int max_events;
		int max_listen;
		int accept_timeout;
		int handle_timeout;
		bool F_running = false;
		bool F_stop = false;
		bool F_pause = false;
		bool F_master_fail = false;
		bool F_reset_callable = false;
		bool F_listening = false;


	}; // class Device
} // namespace IODevice
} // namespace ScrybeIO

#endif
