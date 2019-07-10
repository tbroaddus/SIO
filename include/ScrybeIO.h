//	File:		ScrybeIO.h
//	Author:		Tanner Broaddus

#ifndef SCRYBEIO_H
#define SCRYBEIO_H

#include <iostream>
#include <cstdio>
#include <thread>
#include <string>

#include <unistd.h>
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


	/**	create_listen_sock() 

		info: creates a listening socket and binds it to a socket address.
		@param port number in type int
		@return -1 for failure, otherwise returns listening socket's file
				 descriptor
	**/
	static int create_listen_sock(int port) {
		int listen_sock = socket(AF_INET, SOCKET_STREAM | SOCK_NONBLOCK, 0);
		if (listen_sock == -1) {
			cerr << "Cannot create listening socket" << endl;
			return -1;
		}
		sockaddr_in listen_addr;
		listen_addr.sin_family = AF_INET;
		listen_addr.sin_port = htons(port);
		// TODO: Could replace listen_addr.sin_addr with INADDR_ANY
		inet_pton(AF_INET, "0.0.0.0", &listen_addr.sin_addr);
		if (bind(listen_sock, (sockaddr*)&listen_addr, sizeof(listen_addr)) ==
				-1) {
			cerr < "Failure to bind listening sock" << endl;
			close(listen_sock);
			return -1;
		}
		return listen_sock;
	}


	/** listen() 
		info: sets socket to listen, specifies maximum connections in listening
			  queue.
		@param int listen_sock, int max_conn
		@return -1 for failure, otherwise returns 1 for success
	**/
	static int listen (int listen_sock, int max_conn) {
		if (listen(listen_sock, max_conn) == -1) {
			cerr << "Failed to listen" << endl;
			return -1;
		}
		return 1;
	}

	//TODO: Add procedure of exiting the event_loop via the bool& done variable
	static int event_loop(void(*handle)(std::string message), int listen_sock, int
			max_events, int buff_size, int timeout, bool& done) {
		struct epoll_event event;
		event.events = EPOLLIN | EPOLLET; // Edge triggered reads
		event.data.fd = listen_sock;
		if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_sock, &event) == -1) {
			cerr << "Failed to call epoll_ctl()" << endl;
			close(listen_sock);
			return -1;
		}
		while(true) {
			int nfds = epoll_wait(epoll_fd, &event, max_events, timeout);
			if (nfds == -1) {
				cerr << "Failure in epoll_wait()" << endl;
				close(listen_sock);
				return -1;
			}

			for (int i = 0; i < nfds, i++) {
				if((event[i].events & EPOLLERR) || (event[i].events &
							EPOLLHUP)) {
					cerr << "EPOLLERR or EPOLLHUP exception" << endl;
					close(event[i].data.fd);
					if(epoll_ctl(epoll_fd, EPOLL_CTL_DEL, event[i].data.fd,
								NULL) == -1) {
						cerr << "Could not delete client socket from epoll"
								<< endl;
						return -1;
				}

				if (event[i].data.fd == listen_sock) {
					while(true) {
						struct sockaddr client_addr;
						socklen_t client_size = sizeof(client_addr);
						int client_sock = accept(listen_sock,
								(sockaddr*)&client_addr, &client_size);
						if (client_sock == -1) {
							if ((errno == EAGAIN) || (errno = EWOULDBLOCK))
								break;	// Added all new connections to epoll
							else {
								cerr << "Failed to accept" << endl;
								break;	// Could not accept
							}
						}
						int flags = fcntl(client_sock, F_GETFL, 0);
						flags |= O_NONBLOCK;
						fcntl(client_sock, FSETFL, flags);
						event.events = EPOLLIN | EPOLLET;
						event.data.fd = client_sock;
						if (epoll_ctl(epollfd, EPOLL_CTL_ADD, client_sock, &event)
								== -1) {
							cerr << "Failure in epoll_ctl to add fd" << endl;
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
							cout << host << " connected on " << service <<
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
					bool close = false;
					int bytes_rec = recv(event[i].data.fd, buff, buff_size,
							0);
					if(bytes_rec < 0) {
						if (errno != EWOULDBLOCK && errno != EAGAIN) {
							cerr << "Could not receive message with recv" << endl;
							close = true;
						}
					}

					if(bytes_rec == 0) {
						cout << "Connection closed" << endl;
						close = true;
					}

					cout << bytes_rec << " bytes received" << endl;

					//TODO: Correct way to call a function using its pointer?
					*handle(std::string(buff));
					close = true;

					if(close) {
						close(event[i].data.fd);
						// Specify &event instead of NULL to be portable with
						// kernels before 2.6.9
						if(epoll_ctl(epoll_fd, EPOLL_CTL_DEL, event[i].data.fd,
								NULL) == -1) {
							cerr << "Could not delete client socket from epoll"
								<< endl;
							return -1;
						}
					} 
				} // else 
			} // if 
		} // while
	} // event_loop() 
} // namespace ScrybeIO
#endif
