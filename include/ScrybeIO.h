//	File:		ScrybeIO.h
//	Author:		Tanner Broaddus

#ifndef	SCRYBEIO_H 
#define	SCRYBEIO_H 

#include <iostream>
#include <cstdio>
#include <thread>
#include <string>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>

#include "ScrybeIOProtocols.h"
#include "LockQueue.h"

// Parameters to be set
#define THREADCOUNT 4		//	Number of desired threads (1 thread always
							//	dedicated to listening)
#define MAXEVENTS 10		//	Max events returned per handler thread
#define LISTENTIMEOUT 100	//	in milliseconds
#define HANDLETIMEOUT 100	//	in milliseconds

using std::cout;
using std::endl;
using std::cerr;

namespace ScrybeIO {

class IODevice {
	
	public:

		explicit IODevice(void(*_handle_accept)(int sock) , int _port) {
			handle_accept = &_handle_accept;
			port = _port;
			stop = false;
			finish = false;
		}

		~IODevice() {

		}

		int Start() {

			switch(THREADCOUNT) {

				case 2: 
					std::thread L1(Listen);
					std::thread H1(Handle);
					while(true) {
						input = getchar();
						if (input == 'q') {
							stop = true;
							L1.join();
							H1.join();
							return 0;
						}
					}
				
				case 3:
					std::thread L1(Listen);
					std::thread H1(Handle);
					std::thread H2(Handle);
					while(true) {
						input = getchar();
						if (input == 'q') {
							stop = true;
							L1.join();
							H1.join();
							H2.join();
							return 0;
						}
					}

				case 4:	
					std::thread L1(Listen); 
					std::thread H1(Handle);
					std::thread H2(Handle);
					std::thread H3(Handle);
					while(true) {
						input = getchar();
						if (input == 'q') {
							stop = true;
							L1.join();
							H1.join();
							H2.join();
							H3.join();
							return 0;
						}
					}

				default:
					cerr << "Invalid THREADCOUNT... Select in range 2-4" << endl;
					return -1;
			}
		}

	private:

		// Listening thread
		void Listen() {
			int listen_sock = socket(AF_INET, SOCKET_STREAM | SOCK_NONBLOCK, 0);
			if (listen_sock == -1) {
				cerr << "Cannot create listening socket" << endl;
			}
			
			sockaddr_in listen_addr;
			listen_addr.sin_family = AF_INET;
			listen_addr.sin_port = htons(port);
			inet_pton(AF_INET, "0.0.0.0", &listen_addr.sin_addr);

			if (bind(listen_soc,
						(sockaddr*)&listen_addr,sizeof(listen_addr)) == -1) {
				cerr << "Failure to bind listening sock" << endl;
				close(listen_sock);
			}

			if(listen(listen_sock, MAXCONNECTIONS) == -1) {
				cerr << "Failed to listen" << endl;
				close(listen_sock);
			}

			int epoll_fd = epoll_create1(0);
			if (epoll_fd == -1) {
				cerr << "Failed to create epoll" << endl;
				close(listen_sock);
			}

			struct epoll_event event;
			event.events = EPOLLIN | EPOLLET; // Edge triggered read events
			event.data.fd = listen_sock;
			if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_sock, &event) ==
						-1) {
				cerr << "Failed to call epoll_ctl()" << endl;
				close(listen_sock);
			}

			while(true) {
				int nfds = epoll_wait(epoll_fd, &event, 1, LISTENTIMEOUT);
				if (nfds == -1)	{
					cerr << "Failure in epoll_wait()";
					close(listen_sock);
				}

				if (nfds == 1) {
					while(true) {
						sockaddr_in client;
						socklen_t clientSize = sizeof(client);
						int client_sock = accept(listen_sock,
								(sockaddr*)&client, &clientSize);
						if (client_sock == -1) {
							if ((errno == EAGAIN) || (errno = EWOULDBLOCK)) 
								break; // No more new connections.
							else {
								cerr << "Failed to accept" << endl;
								break;
							}
						}
						// Setting client_sock to non-blocking
						int flags = fcntl (client_sock, F_GETFL, 0);
						flags |= O_NONBLOCK;
						fcntl (client_sock, F_SETFL, flags);
						IO_fd_queue.push(client_sock); // client_sock pushed to
													   // LockQueue
					}
				}

				//TODO: Create a better statement body for this if
				//		statement.	
				if (stop) {
					finish = true;
					break;
				}
			}
			close(listen_sock);
		}
		//TODO: Handle() is going to need a little more work towards the end...		
		// Handling thread(s)
		void Handle() {
			int epoll_fd = epoll_create1(0);
			if (epoll_fd == -1) 
				cerr << "Failed to create handle epoll" << endl;
			struct epoll_event event;
			event.events = EPOLLIN | EPOLLET; // Edge triggered read events

			while(true) {
				if (!IO_fd_queue.empty() || finish != true) {
					int new_sock = IO_fd_queue.pop();
					if (new_sock != EMPTY) {	// Queue could now be empty,
												// must check.
						event.data.fd = new_sock;
						if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_sock, &event)
								== -1) {
							cerr << "Failed to call epoll_ctl() in handle" <<
								endl;
							close(new_sock);
						}
					}
				}
				//TODO: Find a value to replace this 1 with
				int nfds = epoll_wait(epoll_fd, &event, MAXEVENTS, HANDLETIMEOUT);
				for (int i = 0; i < n; i++) {
					int client_sock = events[i].data.fd;

					/*
					   May have to actually read the message and pass it to
					   handle_accept instead of passing the socket fd. There 
					   could be a EWOULDBLOCK or EAGAIN exception with no easy way to handle
					   it in handle_accept, best to handle it here. we can pass
					   the message and deserialize it in handle_accept.
				    */

					*handle_accept(client_sock); //TODO: do we need the *
												 //		before?
					//TODO: Close sock? Remove from epoll?
					close(client_sock); // Check to see if you would want to
										// this here. 
				}
				//TODO: Is this proper? Could poll over sockets
				//		one last time...
				if (IO_fd_queue.empty() && finish == true)
					break;
			}
		}

		// Private member variables
		LockQueue IO_fd_queue;
		int port;
		bool stop;
		bool finish;
		void (*handle_accept)(int sock);
};
}
#endif
