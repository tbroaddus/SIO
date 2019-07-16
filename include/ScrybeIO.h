//	File:		ScrybeIO.h
//	Author:		Tanner Broaddus

#ifndef SCRYBEIO_H
#define SCRYBEIO_H

#include <iostream>
#include <cstdio>
#include <thread>
#include <string>

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

namespace ScrybeIO {

// TODO: Have separate namespace for these functions


	// create_listen_sock()
	/**	
	  info: 
		Creates a listening socket and binds it to a socket address.
		User provides desired local port and max number of connection requests
		in the listening queue.
	  @param int port
	  @param max_conn
	  @return -1 for failure, otherwise listening socket fd
	**/
	static int create_listen_sock(int port, int max_conn) {
		int listen_sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
		if (listen_sock == -1) {
			cerr << "Cannot create listening socket\n";
			return -1;
		}
		sockaddr_in listen_addr;
		listen_addr.sin_family = AF_INET;
		listen_addr.sin_port = htons(port);
		// TODO: Could replace listen_addr.sin_addr with INADDR_ANY
		inet_pton(AF_INET, "0.0.0.0", &listen_addr.sin_addr);
		if (bind(listen_sock, (sockaddr*)&listen_addr, sizeof(listen_addr)) ==
				-1) {
			cerr << "Failure to bind listening sock\n";
			close(listen_sock);
			return -1;
		}
		if (listen(listen_sock, max_conn) == -1) {
			cerr << "Failed to listen\n";	
			return -1;
		}
		return listen_sock;
	}
	
	//TODO: Add procedure for exiting the event_loop via the bool& done variable
	// event_loop()
	/**
	  info: 
		Starts an IO event loop implemented using EPOLL.
		User provides a function to handle client requests, listening socket
		file descriptor, max number of events returned from epoll_create(),
		message buffer size, timeout period (in milliseconds), a bool variable
		for the function stop procedure.
	  @oaram void(*f)(arg1, arg2,...argn)
	  @param int listen_sock
	  @oaram int max_events
	  @param int buff_size
	  @param int timeout 
	  @param bool& done
	  @return -1 for failure, 1 for success
	**/
	static int event_loop(void(*handle)(std::string message), int listen_sock, int
			max_events, int buff_size, int timeout, bool& done) {
		int epoll_fd = epoll_create1(0);
		if (epoll_fd == -1) {
			cerr << "Failed to create epoll\n";
			close(listen_sock);
			return -1;
		}

		struct epoll_event event, events[max_events];
		event.events = EPOLLIN | EPOLLET; // Edge triggered reads
		event.data.fd = listen_sock;
		if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_sock, &event) == -1) {
			cerr << "Failed to call epoll_ctl()\n";
			close(listen_sock);
			return -1;
		}
		
		while(true) {
			// cout for testing purposes
			cout << "timed out: new event loop!" << endl;
			int nfds = epoll_wait(epoll_fd, events, max_events, timeout);
			if (nfds == -1) {
				cerr << "Failure in epoll_wait()\n";
				close(listen_sock);
				return -1;
			}

			for (int i = 0; i < nfds; i++) {
				if((events[i].events & EPOLLERR) || (events[i].events &
							EPOLLHUP)) {
					cerr << "EPOLLERR or EPOLLHUP exception\n";
					// close(events[i].data.fd); NOT NEEDED
					if(epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd,
								NULL) == -1) {
						cerr << "Could not delete client socket from epoll\n";
								
						return -1;
					}
				}

				if (events[i].data.fd == listen_sock) {
					while(true) {
						struct sockaddr_in client_addr;
						socklen_t client_size = sizeof(client_addr);
						int client_sock = accept(listen_sock,
								(sockaddr*)&client_addr, &client_size);
						if (client_sock == -1) {
							if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
								break;	// Added all new connections to epoll
							else {
								cerr << "Failed to accept\n";
								break;	// Could not accept
							}
						}
						int flags = fcntl(client_sock, F_GETFL, 0);
						flags |= O_NONBLOCK;
						fcntl(client_sock, F_SETFL, flags);
						event.events = EPOLLIN | EPOLLET;
						event.data.fd = client_sock;
						if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_sock, &event)
								== -1) {
							cerr << "Failure in epoll_ctl to add fd\n"; 
							return -1;
						}
						// For testing purposes
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
					}
				}
				else {
					// TODO while(true)?
					char buff[buff_size];
					memset(buff, 0, buff_size);
					bool close_fd = false;
					int bytes_rec = recv(events[i].data.fd, buff, buff_size,
							0);
					if(bytes_rec < 0) {
						if (errno != EWOULDBLOCK && errno != EAGAIN) {
							cerr << "Could not receive message with recv\n";
							close_fd = true;
						}
					}

					if(bytes_rec == 0) {
						cout << "Connection closed" << endl;
						close_fd = true;
					}

					cout << bytes_rec << " bytes received" << endl;
					// handle_accept from IOTest.cpp
					(*handle)(std::string(buff));
					close_fd = true;

					//TODO: useless if statement if no infinite while()
					if(close_fd) {
						// Specify &event instead of NULL to be portable with
						// kernels before 2.6.9 if that is a concern.
						if(epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd,
								NULL) == -1) {
							cerr << "Could not delete client socket from epoll\n";
								
							return -1;
						}
					} 
				} // else 
			} // if
		   // TODO: Does anything need to be done for clean up?
		   if(done) {
			   return 1; // BE SURE TO CLOSE LISTEN SOCK FROM CALLING FUNCTION
		   }
		} // for 
	} // event_loop() 
} // namespace ScrybeIO
#endif
