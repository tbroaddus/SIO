//	File:		test_device.cpp
//	Author:		Tanner Broaddus
//	Notice:		If using GNU compiler, include -pthread when compiling

#include <iostream>
#include <string>
#include <thread>
#include <unistd.h>
#include <stdlib.h>

#include "IO_Device.h"

using std::cout;
using std::endl;
using std::cerr;


// Represents handle_accept function from Scrybe in Server.cpp
void handle_accept(std::string request) {
	cout << "From Handle_accept() in test_device.cpp!" << endl;
	cout << request << endl;
}

int main() {

	// Passing in handle_accept pointer along with a port number (54000)
	ScrybeIO::IODevice IODev(&handle_accept, 54000);

	// set_listen() creates a socket, binds it to a socket
	// address, and sets it to listen for new connections.
	// New connections will be in the listening queue before start() is called.
	int result_listen = IODev.set_listen();
	switch(result_listen)
	{
		case 0:
			cout << "Already running" << endl;
			break;
		case -1:
			cerr << "socket() failed)\n";
			return 0;
		case 1:
			cout << "Success in making listening socket!" << endl;
			break;
	}
	// start() creates acceptor and handler threads. Acceptor uses its own
	// epoll loop to observe the listening socket to see if there are any new
	// connections to be added. If so, the acceptor thread creates a client
	// socket and adds it to the epoll structure shared by the handler threads
	// where the client's request will be handled. 
	int result_start = IODev.start();
	switch(result_start)
	{
		case 0:
			cout << "Already running" << endl;
			break;
		case -1:
			cerr << "Fatal Error\n";
			return 0;
		case 1:
			cout << "Success in starting IODev" << endl;
			break;
	}
	
	// stop() can be called at any time so if additional work needs to be done
	// between start and stop, it is feasible to do so.

	// -- Additional work --
	// ...
	// ...
	// ...

	while(true) {
		char input = getchar();
		if (input == 'q') {
			IODev.stop();
			break;
		}
	}

	if(IODev.reset() == -1) {
		cerr << "Could not reset" << endl;
	}

	result_listen = IODev.set_listen();
	switch(result_listen)
	{
		case 0:
			cout << "Already running" << endl;
			break;
		case -1:
			cerr << "socket() failed)\n";
			return 0;
		case 1:
			cout << "Success in making listening socket!" << endl;
			break;
	}

	result_start = IODev.start();
	switch(result_start)
	{
		case 0:
			cout << "Already running" << endl;
			break;
		case -1:
			cerr << "Fatal Error\n";
			return 0;
		case 1:
			cout << "Success in starting IODev" << endl;
			break;
	}
	while(true) {
		char input = getchar();
		if (input == 'q') {
			IODev.stop();
			break;
		}
	}

	return 0;
}
