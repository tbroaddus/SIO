//	File:		IO_Device.h
//	Author:		Tanner Broaddus

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

#include "ScrybeIO.h"

using std::cout;
using std::endl;
using std::cerr;


//					MACROS
// --------------------------------------------
#define THREADCOUNT 4
#define MAXEVENTS 10
#define MAXLISTEN 200
#define LISTENTIMEOUT 100	//	in milliseconds
#define HANDLETIMEOUT 100	//	in milliseconds
// --------------------------------------------


namespace ScrybeIO {
namespace Server {

//IODevice type
class IODevice {

	public:
		explicit IODevice(void(*_handle)(std::string message), int _port)
			{
				handle = _handle;
				port = _port;
				stop = false;
				finish = false;
		} // IODevice() constructor
	

		// TODO: needed?
		~IODevice() {}


		// listen()
		/**
			info:
				Creates, binds, and starts listening socket. Should only be
				called once.
				@param NULL
				@return -1:failure, 1:success
		**/
		int listen() {
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
			return 1;
		} // listen()


		// start()
		int start() {
			epoll_fd = epoll_create1(0);
			if (epoll_fd == -1) {
				cerr << "Failure in epoll_create1(0)\n";
				close(listen_sock);
				return -1;
			}
		// TODO: FIGURE THIS SH*T OUT
			for(int i = 0; i < THREADCOUNT-1; i++) {

			}
			return 1;
		} // start() 


	private:

		// struct acceptor 
		struct acceptor() {

			void accept_conns() {

			}

			bool fail = false;
		} // struct acceptor 


		// struct handler
		struct handler {

			void handle_conns() {
				
			}

			bool fail = false;
		} // struct handler

		// TODO: FIGURE THIS SH*T OUT
		const std::shared_ptr& create_handler() {
			return 	
		}

		// TODO: FIGURE THIS SH*T OUT
		const std::thread& create_thread(struct handler& handler_ref) {
			return new std::thread(handler_ref.handle_conns);



		// Private member variables
		void (*handle)(std::string request);
		std::vector<handler> handler_vec;
		std::vector<std::thread> thread_vec;
		struct acceptor acceptor_L1;
		int listen_sock;
		int epoll_fd;
		int port;
		bool stop;
		bool finish;

}; // class IODevice

} // namespace Server
} // namespace ScrybeIO
