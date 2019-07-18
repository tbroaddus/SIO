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

void handle_accept(std::string request) {
	cout << "From Handle_accept() in test_device.cpp!" << endl;
	cout << request << endl;
}

int main() {
	ScrybeIO::IODevice IODev(&handle_accept, 54000);
	int result_listen = IODev.set_listen();
	switch(result_listen)
	{
		case 0:
			cout << "Already running" << endl;
			break;
		case -1:
			cerr << "socket() failed)\n";
			exit(0);	
		case 1:
			cout << "Success in making listening socket!" << endl;
			break;
	}
	int result_start = IODev.start();
	switch(result_start)
	{
		case 0:
			cout << "Already running" << endl;
			break;
		case -1:
			cerr << "Baaaad\n";
			exit(0);	
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
