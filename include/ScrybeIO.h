//	File:		ScrybeIO.h
//	Author:		Tanner Broaddus

#ifndef	SCRYBEIO_H 
#define	SCRYBEIO_H 

#include <iostream>
#include <thread>
#include <string>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>


#include "ScrybeIOProtocols.h"
#include "LockQueue.h"

using std::cout;
using std::endl;
using std::cerr;

namespace ScrybeIO {

	class IODevice {
		
		public:

			explicit IODevice(void(*_handle)(Socket sock) , int _port) {
				handle = &_handle;
				port = _port;
				stop = false;
			}

			~IODevice() {

			}

			Start() {

			

			Stop() {

			}


		private:

			int Listen() {
				int listen_sock = socket(AF_INET, SOCKET_STREAM, 0);
				if (listen_sock == -1) {
					cerr << "Cannot create listening socket" << endl;
					return -1;
				}
				
				sockaddr_in listen_addr;
				listen_addr.sin_family = AF_INET;
				listen_addr.sin_port = htons(port);
				inet_pton(AF_INET, "0.0.0.0", &listen_addr.sin_addr);

				 if (bind(listen_soc,
							(sockaddr*)&listen_addr,sizeof(listen_addr)) ==
						-1) {
					cerr << "Failure to bind listening sock" << endl;
					return -2;
				}

			}

			void Handle() {

			}

			LockQueue IO_fd_queue;
			int port;
			bool stop;
			bool finish;
			void (*handle)(Socket sock);



			
	};
}


#endif

