//	File:		test_device.cpp
//	Author:		Tanner Broaddus
//	Notice:		If using GNU compiler, include -pthread when compiling

#include <iostream>
#include <string>
#include <thread>
#include <unistd.h>
#include <stdlib.h>
#include <chrono>
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

	// reset() can be called while the device is not running. This call resets
	// all variables to their default state. Intended to be used after a crash.
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

	// resume() can be called after stop() has been called to continue handling
	// requests and accepting new client connections. 	
	if (IODev.resume() == -1) {
		cerr << "Failure in IODev.resume()" << endl;
		return 0;
	}
	cout << "Resume() succesful!" << endl;

	// --- Work ----
	// ...
	// ...
	// ...

	// "Guantlet" of inappropriate IODevice calls by the user 
	std::this_thread::sleep_for(std::chrono::seconds(5));

	if (IODev.reset() == -1)
		cerr << "Cannot call reset\n";

	std::this_thread::sleep_for(std::chrono::seconds(1));

	if (IODev.start() == -1)
		cerr << "Cannot call start\n";

	std::this_thread::sleep_for(std::chrono::seconds(1));

	if (IODev.set_listen() == -1)
		cerr << "Cannot call listen\n";


	while(true) {
		char input = getchar();
		if (input == 'q') {
			int stop_res = IODev.stop();
			if (stop_res == -1)
			   cerr << "master fail\n";	
			break;
		}
	}


	std::this_thread::sleep_for(std::chrono::seconds(1));

	// Another resume() call followed by another "Guantlet"
	if(IODev.resume() == -1) {
		cerr << "Failure in second call to IODev.resume()" << endl;
		return 0;
	}
	cout << "Success in second Resume() call!" << endl;

	std::this_thread::sleep_for(std::chrono::seconds(5));

	if (IODev.reset() == -1)
		cerr << "Cannot call reset\n";

	std::this_thread::sleep_for(std::chrono::seconds(1));

	if (IODev.start() == -1)
		cerr << "Cannot call start\n";

	std::this_thread::sleep_for(std::chrono::seconds(1));

	if (IODev.set_listen() == -1)
		cerr << "Cannot call listen\n";

	std::this_thread::sleep_for(std::chrono::seconds(3));

	if(IODev.master_fail() == 1) {
		cout << "Master fail in threads" << endl;
		cout << "Stopping server" << endl;
		IODev.stop();
		return 0;
	}

	cout << "No master fail present!" << endl;

	while(true) {
		char input = getchar();
		if (input == 'q') {
			int stop_res = IODev.stop();
			if (stop_res == -1)
			   cerr << "master fail\n";	
			break;
		}
	}

	if(IODev.resume() == -1) {
		cerr << "Failure in third call to IODev.resume()\n";
		return 0;
	}

	std::this_thread::sleep_for(std::chrono::seconds(2));

	// --- WORK ---	
	// ...
	// ...
	// ...


	// Setting F_master_fail to false to simulate a master fail occuring
	IODev.set_master_fail();

	std::this_thread::sleep_for(std::chrono::seconds(2));
	
	// Program will end here
	if(IODev.master_fail() == 1) {
		cout << "Master fail in threads" << endl;
		cout << "Stopping server" << endl;
		IODev.stop();
		return 0;
	}

	while(true) {
		char input = getchar();
		if (input == 'q') {
			int stop_res = IODev.stop();
			if (stop_res == -1)
			   cerr << "master fail\n";	
			break;
		}
	}
	return 0;
}
