//	File:		IO_Device.h
//	Author:		Tanner Broaddus
/*
	Notes:

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
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <sys/socket.h>
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
#define ACCEPT_TIMEOUT 100	//	in milliseconds
#define HANDLE_TIMEOUT 100	//	in milliseconds
// --------------------------------------------


namespace ScrybeIO {


//IODevice type
class IODevice {


//					Public
//-------------------------------------------------------------------------------------

	public:


		explicit IODevice(void(*_handle)(std::string message), int _port)
			{
				handle = _handle;
				port = _port;
		} // IODevice() constructor
	

		// TODO: needed?
		~IODevice() {}


		// listen()
		/**
			info:
				Creates, binds, and starts listening socket. Should only be
				called once.
				@param NULL
				@return -1:failure, 1:success, 0:running
		**/
		int listen() {
			if (F_running == true) {
				cout << "IO device already listening and running" << endl;
				return 0;
			}
			F_reset_callable = false;
			listen_sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
			if (listen_sock == -1) {
				cerr << "Cannot create listening socket\n";
				return -1;
			}
			sockaddr_in listen_addr;
			listen_addr.sin_family = AF_INET;
			listen_addr.sin_port = htons(port);
			inet_pton(AF_INET, INADDR_ANY, &listen_addr.sin_addr);
			if (bind(listen_sock, (sockaddr*)&listen_addr, sizeof(listen_addr))
					== -1 {
					cerr << "Failure to bind listening sock\n";
					close(listen_sock);
					return -1;
			}
			if (listen(listen_sock, MAXLISTEN) == -1) {
				cerr << "Failed to listen\n";
				return -1;
			}
			F_listening = true;
			return 1;
		} // listen()


		// TODO: Keep track of running
		// start()
		const int start() {
			if (F_listening == false) {
				cerr << "Must call listening() before calling start()\n";
				return -1;
			}
			if (F_running == true) {
				cerr << "IO Device already running\n";
				return 0;
			}
			F_running = true;
			F_stop = false;
			acc_epoll_fd = epoll_create1(0);
			han_epoll_fd = epoll_create1(0);
			if (acc_epoll_fd == -1 || han_epoll_fd == -1) {
				cerr << "Failure in epoll_create1(0)\n";
				close(listen_sock);
				F_running = false;
				return -1;
			}
			
			struct epoll_event event;
			event.events = EPOLLIN | EPOLLET;
			event.data.fd = listen_sock;
			if (epoll_ctl(acc_epoll_fd, EPOLL_CTL_ADD, listen_sock, &event) == -1)
				{
					cerr << "Failed to call epoll_ctl() in start()\n";
					close(listen_sock);
					F_running = false;
					return -1;
			}

			for(int i = 0; i < THREADCOUNT-1; i++) {
				handler_vec.push_back(create_handler());
			}

			std::thread L1(acceptor_L1.accept_conns);
			thread_vec.push_back(L1);

			for(handler _handler : handler_vec) {
				thread_vec.push_back(std::thread(_handler.handle_conns));
			}
			return 1;
		} // start()


		// TODO: Must check if running (bool running)
		// TODO: Must check master fail
		// TODO: Check individual threads for status and return 0 for all
		// running and a postive int for the number that failed.
		// thread_check()
		const int thread_check() const {
			int fail_count;

			return fail_count;

		} // thread_check();

		//TODO: Maybe?
		//failure_report()
		std::string failure_report() const {

		} //failure_report()



		// TODO: Any potential errors to detect?
		// TODO: Check if a thread failed, return -1 if so.
		// stop()
		const int stop() {
			if(F_running != true)
				return -1;
			F_stop = true;	
			for(std::thread t : thread_vec) {
				t.join();
			}
			close(listen_sock);
			F_running = false;
			F_reset_callable = true;
			return 1;
		} // stop()



		// reset()
		const int reset() {
			if (F_reset_callable == false)
				return -1;
			handler_vec.empty();
			thread_vec.empty();
			struct acceptor _acceptor;
			acceptor_L1 = _acceptor;
			F_listening = false;
			F_running = false;
			F_stop = false;
			F_master_fail = false;
			return 1;
		} // reset()


	//						Private
	//-------------------------------------------------------------------------------------
	private:

		/*
		   FOR THE LOVE OF NEPTUNE, DO NOT CLOSE THE LISTEN_SOCK IN A THREAD...
		*/
		// TODO: MAKE THIS
		// struct acceptor 
		struct acceptor {

			void accept_conns() {
				struct epoll_event events[1]; // One because we are only wanting
				                              // to observe the listening
											  // socket.
				// Main event loop
				while(true) {
					if (F_master_fail == true || F_stop == true)
						break;
					if (acc_fail == true) {
						F_master_fail = true;
						break;
					}
					// Checking to see if handler threads are still
					// functioning.
					int han_fail_count = 0;
					for (handler _handler : handler_vec) {
						if (_handler.F_han_fail = true)
							han_fail_count++;
					}
					if (han_fail_count == THREADCOUNT - 1) {
						F_master_fail = true;
						break;
					}
					int nfds = epoll_wait(acc_epoll_fd, events, 1, ACCEPT_TIMEOUT);
					if (nfds == -1) {
						cerr << "Failure in epoll_wait() in acceptor\n";
						F_acc_fail = true;
					}
					if (nfds > 0) {
						// Accept loop
						while (true) {
							struct sockaddr_in client_addr;
							socklen_t client_size = sizeof(client_addr);
							int client_sock = accept(listen_sock,
									(sockaddr*)&client_addr, &client_size);
							if (client_sock == -1) {
								if ((errno == EAGAIN || errno == EWOULDBLOCK))
									break; // Added all new connections 
								else {
									cerr << "Failed to accept\n";
									s_accept_fail++;
									s_accept_loop = -1;
									if (s_accept_fail == ACCEPT_FAIL_LIMIT) {
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
							if (epoll_ctl(han_epoll_fd, EPOLL_CTL_ADD, client_sock,
										&event) == -1) {
								cerr << "Failure in epoll_ctl to add fd\n";
								close(client_sock);
								s_add_fail++;
								s_add_loop = -1;
								if (s_add_fail == ADD_FAIL_LIMIT) {
									F_acc_fail = true;
								}
								break;
							}
							s_accept_loop++;
							s_add_loop++;
							if(s_accept_loop == ACCEPT_LOOP_RESET) 
								s_accept_loop = 0;
							if (s_add_loop == ADD_LOOP_RESET) 
								s_add_loop = 0;

							// Testing purposes 
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
						} // while() 
					} // if () 
				} // while()
			} // accept_conns();

			//struct acceptor public member variables
			int n_accept_fail = 0;
			int n_accept_loop = 0;
			int n_add_fail = 0;
			int n_add_loop = 0;
			bool F_acc_fail = false;

		} // struct acceptor 



		// TODO: MAKE THIS
		// Handlers should NOT invoke a master fail. That is up to the
		// acceptor to do so.
		// struct handler
		struct handler {

			void handle_conns() {
				struct epoll_event events[MAXEVENTS];
				// main event loop	
				while(true) {
					if (F_han_fail == true || F_master_fail == true || F_stop
							== true) 
						break;
					int nfds = epoll_wait(han_epoll_fd, events, 1, HANDLE_TIMEOUT);
					if(nfds == -1) {
						cerr << "Failure in epoll_wait() in handler thread\n";
						F_han_fail = true;
						continue;
					}
					for(int i = 0; i < nfds; i++) {
						if((events[i].events & EPOLLER || (events[i].events &
										EPOLLHUP)) {
							cerr << "EPOLLER or EPOLLERHUP exception in
								handler\n";
							if(epoll_ctl(epoll_fd, EPOLL_CTL_DEL,
										events[i].data.fd, NULL) == -1) {
								cerr << "Could not delete client socket from
									epoll\n";
								F_han_fail = true;
								break;
							}
						}
						char buff[BUFFERSIZE];
						memset(buff, 0, BUFFERSIZE);
						bool close_fd = false;
						int bytes_rec = recv(events[i].data.fd, buff, BUFFERSIZE,
								0);
						if(bytes_rec < 0) {
							if (errno!= EWOULDBLOCK && errno != EAGAIN) {
								cerr << "Could not receive message with\n";
								close_fd = true;
							}
						}
						if (bytes_rec == 0) {
							cout << "Connection closed" << endl;
							close_fd = true;
						}
						cout << bytes_rec << " bytes received" << endl;
						// handle_accept() 
						(*handle)(std::string(buff));
						close_fd = true;
						//TODO: still need to determine if this is needed
						if(close_fd) {
							if(epoll_ctl(han_epoll_fd, EPOLL_CTL_DEL,
										events[i].data.fd, NULL) == -1) {
								cerr << "Could not delete client socket from
									epoll\n";
								F_han_fail = true;
							}
						}
					} // for()
				} // while()
				if (F_han_fail == true) 
					cout << "Thread failed!" << endl;
			}
			bool F_han_fail = false;
		} // struct handler



		// TODO: FIGURE THIS SH*T OUT
		struct handler create_handler() {
			struct handler _handler;
			return _handler; 	
		}



		/*
		// TODO: FIGURE THIS SH*T OUT
		const std::thread& create_thread(struct handler& handler_ref) {
			return new std::thread(handler_ref.handle_conns);
		*/

		// TODO: Find the size of each of these private members and order
		// accordingly

		// Private member variables
		void (*handle)(std::string request);
		std::vector<handler> handler_vec;
		std::vector<std::thread> thread_vec;
		struct acceptor acceptor_L1;
		std::mutex mu;
		int listen_sock;
		int acc_epoll_fd;
		int han_epoll_fd;
		int port;
		bool F_running = false;
		bool F_stop = false;
		bool F_master_fail = false;
		bool F_reset_callable = false;
		bool F_listening = false;
		

}; // class IODevice
} // namespace ScrybeIO

#endif
