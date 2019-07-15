//	File:		IO_Device.h
//	Author:		Tanner Broaddus

#ifndef IO_DEVICE_H
#define IO_DEVICE_H


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
		explicit IODevice(void(*_handle_conn)(std::string message), int _port);
		int start();	

	private:

		listen();
		handle();
		int port;
		bool stop;
		bool finish;
		void (*handle_conn)(std::string request);

}; // class IODevice

} // namespace Server
} // namespace ScrybeIO

