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


//					MACROS
// --------------------------------------------
#define THREADCOUNT 4 
#define BUFFERSIZE 4096
#define ACCEPT_FAIL_LIMIT 5
#define ACCEPT_LOOP_RESET 10
#define ADD_FAIL_LIMIT 5
#define ADD_LOOP_RESET 10
#define MAXEVENTS 10
#define MAXLISTEN 200
#define ACCEPT_TIMEOUT 1000	//	in milliseconds
#define HANDLE_TIMEOUT 1000	//	in milliseconds
// --------------------------------------------


namespace ScrybeIO {


	
// IODevice type
class IODevice {
	

//					Public
//-------------------------------------------------------------------------------------

	public:


		// Constructor
		explicit IODevice(void(*_handle)(std::string request, int client_sock), int _port)
		{
			handle = _handle;
			port = _port;
		} 


		// Destructor
		~IODevice() {
			close(listen_sock);
		}

		
		
		// Acceptor and handler now have access to IODevice's private members
		friend struct acceptor;
		friend struct handler;

		



		// --- User Functions ---
		// - set_listen()
		// - start()
		// - stop()
		// - pause()
		// - resume()
		// - reset()
		// - check_master_fail()


		// set_listen()
		const int set_listen() {
			if (F_running == true) {
				cerr << "IO device already listening and running\n";
				return -1;
			}
			if (F_stop == true || F_pause == true) {
				cerr << "Must call reset() before set_listen()\n";
				return -1;
			} 
			F_reset_callable = false;
			listen_sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
			if (listen_sock == -1) {
				cerr << "Cannot create listening socket\n";
				return -1;
			}

			int set_sock = 1;
			// Allows bind() without error after reset() call		
			if (setsockopt(listen_sock, SOL_SOCKET, SO_REUSEPORT, &set_sock,
					sizeof(set_sock)) == -1) {
				cerr << "Failure in setsockopt() in IO_Device\n";
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
					cerr << "Failure to bind listening sock\n";
					close(listen_sock);
					F_reset_callable = true;
					return -1;
			}
			if (listen(listen_sock, MAXLISTEN) == -1) {
				cerr << "Failed to listen\n";
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
				cerr << "Must call listening() before calling start()\n";
				return -1;
			}
			if (F_running == true) {
				cerr << "IO Device already running\n";
				return -1;
			}
			if (F_pause == true) {
				cerr << "Must call reset(), set_listen() before start()\n";
				return -1;
			}
			F_running = true;
			F_stop = false;
			acc_epoll_fd = epoll_create1(0);
			han_epoll_fd = epoll_create1(0);
			if (acc_epoll_fd == -1 || han_epoll_fd == -1) {
				cerr << "Failure in epoll_create1(0)\n";
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
					cerr << "Failed to call epoll_ctl() in start()\n";
					close(listen_sock);
					F_listening = false;
					F_running = false;
					F_reset_callable = true;
					return -1;
			}

			for(int i = 0; i < THREADCOUNT-1; i++) {
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
				cerr << "Stop() called while not running\n";
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
				cerr << "Master fail detected in stop()\n";

			}
		
			return 0;
		} // stop()


	


		// pause()
		const int pause() {
			if (F_running != true) {
				cerr << "Pause() called while not running\n";	
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
				cerr << "Master fail detected in pause()\n";
				return -1;
			}

			return 0;
		} // pause() 





		// resume()
		const int resume() {
			if (F_pause != true)  {
				cerr << "Device was not paused when resume() was called\n";
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





		// reset()
		const int reset(int _port = -1) {
			if (F_reset_callable == false) {
				cerr << "reset() not callable\n";
				return -1;
			}
			if (_port != -1)
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
		} // reset()




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




	//						Private
	//-------------------------------------------------------------------------------------

	private:


		// struct acceptor 
		struct acceptor {

			void accept_conns(IODevice& IODev) {
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
					if (han_fail_count == THREADCOUNT - 1) {
						IODev.F_master_fail = true;
						continue;
					}
					int nfds = epoll_wait(IODev.acc_epoll_fd, events, 1, ACCEPT_TIMEOUT);
					if (nfds == -1) {
						cerr << "Failure in epoll_wait() in acceptor\n";
						F_acc_fail = true;
						continue;
					}
					if (nfds > 0) {
						// Accept loop
						while (true) {
							if (n_accept_loop == ACCEPT_LOOP_RESET) {
								n_accept_fail = 0;
								n_accept_loop = 0;
							}
							if (n_add_loop == ADD_LOOP_RESET) {
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
									cerr << "Failed to accept new client\n";
									n_accept_fail++;
									n_accept_loop++;
									if (n_accept_fail == ACCEPT_FAIL_LIMIT) {
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
								cerr << "Failure in epoll_ctl to add fd\n";
								close(client_sock);
								n_add_fail++;
								n_add_loop++;
								if (n_add_fail == ADD_FAIL_LIMIT) {
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

			void handle_conns(IODevice& IODev) {
				struct epoll_event events[MAXEVENTS];
				// main event loop	
				while(true) {
					// cout << "Handler loop executing" << endl;
					if (F_han_fail == true || IODev.F_master_fail == true ||
							IODev.F_stop == true || IODev.F_pause == true) 
						break;
					int nfds = epoll_wait(IODev.han_epoll_fd, events, MAXEVENTS, HANDLE_TIMEOUT);
					if (nfds == -1) {
						cerr << "Failure in epoll_wait() in handler thread\n";
						F_han_fail = true;
						continue;
					}
					for(int i = 0; i < nfds; i++) {
						if ((events[i].events & EPOLLERR) || (events[i].events &
										EPOLLHUP)) {
							cerr << "EPOLLERR in handler\n";
							if (epoll_ctl(IODev.han_epoll_fd, EPOLL_CTL_DEL,
										events[i].data.fd, NULL) == -1) {
								cerr << "Could not delete client socket from epoll\n";
								F_han_fail = true;
								break;
							}
							close(events[i].data.fd);
							continue;
						}
						bool close_fd = false;
						char buff[BUFFERSIZE];
						memset(buff, 0, BUFFERSIZE);

						while (true) {
							int bytes_rec = recv(events[i].data.fd, buff, BUFFERSIZE,
									0);
							if (bytes_rec < 0) { 
								if (errno!= EWOULDBLOCK && errno != EAGAIN) {
									// cout << errno << endl;
									cerr << "Could not receive message\n";
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
								cerr << "Could not delete client socket from epoll\n";
								F_han_fail = true;
							}
							close(events[i].data.fd);
						}
					} // for()
				} // while()
				if (F_han_fail == true) 
					cerr << "Handler thread failed\n";
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
		bool F_running = false;
		bool F_stop = false;
		bool F_pause = false;
		bool F_master_fail = false;
		bool F_reset_callable = false;
		bool F_listening = false;

	}; // class IODevice
} // namespace ScrybeIO

#endif
