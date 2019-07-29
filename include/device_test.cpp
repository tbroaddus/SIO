//	File:		device_test.cpp
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
void handle_accept(std::string request, int client_sock) {
	// cout << "From Handle_accept() in test_device.cpp!" << endl;
	// cout << request << endl;
	int sendRes = send(client_sock, request.c_str(), request.size() + 1, 0); 	
	if (sendRes == -1) {
		cout << "Could not send to client!" << endl;
	}
	
/*	if (sendRes)
		cout << "Sent to " << client_sock << endl;
*/	
}

int main() {
	cout << "\n\n===========================" << endl;;
	cout << "ScrybeIO Device test...  ||" << endl;
	cout << "===========================\n\n";


	ScrybeIO::IODevice::Options Dev_opt;
	
	// Defining option variables
	Dev_opt.port = 54000;
	Dev_opt.thread_count = 4;
	Dev_opt.buffer_size = 4096;
	Dev_opt.accept_fail_limit = 1;
	Dev_opt.accept_loop_reset = 10;
	Dev_opt.add_fail_limit = 1;
	Dev_opt.add_loop_reset = 10;
	Dev_opt.max_events = 10;
	Dev_opt.max_listen = 100;
	Dev_opt.accept_timeout = 1000;
	Dev_opt.handle_timeout = 1000;

	ScrybeIO::IODevice::Device IODev(&handle_accept, Dev_opt);

	if(IODev.set_listen() == -1) {
		cout << "Could not call set_listen()" << endl;
		return 0;
	}

	if(IODev.start() == -1) {
		cout << "Could not call start()" << endl;
		return 0;
	}
	cout << "\nDevice constructed and running..." << endl;
	
	std::this_thread::sleep_for(std::chrono::seconds(1));
	cout << "\nPrinting the number of child threads running..." << endl;
	cout << "There are " << IODev.n_running() << " threads running" << endl;
	std::this_thread::sleep_for(std::chrono::seconds(1));

	cout << "\nEnter q to stop device: ";
	while(true) {
		char input = getchar();
		if (input == 'q') {
			IODev.stop();
			break;
		}
	}
	cout << endl;
	cout << "Device stopped..." << endl;
	cout << "Reseting device with new options..." << endl;

	Dev_opt.thread_count = 2;
	Dev_opt.accept_timeout = 2000;
	Dev_opt.handle_timeout = 2000;
	IODev.reset(Dev_opt);

	if(IODev.set_listen() == -1) {
		cout << "Could not call set_listen()" << endl;
		return 0;
	}

	if(IODev.start() == -1) {
		cout << "Could not call start()" << endl;
		return 0;
	}

	cout << "\nDevice is running again!" << endl;
	
	cout << "\nPrinting number of child threads..." << endl;
	cout << "There are " << IODev.n_running() << " running" << endl;
	
	cout << "\nEnter q to pause device: ";
	while(true) {
		char input = getchar();
		if (input == 'q') {
			IODev.pause();
			break;
		}
	}
	cout << endl;
	
	std::this_thread::sleep_for(std::chrono::seconds(3));
	cout << "\nDevice is currently paused" << endl;
	cout << "There are " << IODev.n_running() << " threads running" << endl;
	std::this_thread::sleep_for(std::chrono::seconds(1));

	cout << "\nResuming IODev!" << endl;
	if(IODev.resume() == -1) {
		cout << "Could not call resume()" << endl;
		return 0;
	}

	// "Gauntlet" of inappropriate Device calls by the user
	cout << "\nGoing through the \"Gauntlet\" of device calls (Errors expected)" << endl;
	cout << "----------------------------" << endl;
	if (IODev.reset() == -1) {
		cout << "Cannot call reset()" << endl;
	}

	std::this_thread::sleep_for(std::chrono::seconds(1));

	if (IODev.start() == -1) {
		cout << "Cannot call start()" << endl;
	}

	std::this_thread::sleep_for(std::chrono::seconds(1));

	if (IODev.set_listen() == -1) {
		cout << "Cannot call listen()" << endl;
	}
	cout << "----------------------------" << endl;
	cout << endl;


	cout << "\nEnter q to pause device: ";
	while(true) {
		char input = getchar();
		if (input == 'q') {
			int pause_res = IODev.pause();
			if (pause_res == -1) {
			   cout << "master fail\n";	
			   return 0;
			}
			break;
		}
	}
	cout << endl;


	std::this_thread::sleep_for(std::chrono::seconds(1));

	if(IODev.resume() == -1) {
		cout << "Failure in second call to IODev.resume()" << endl;
	}
	cout << "\nDevice resumed!" << endl;


	// Another resume() call followed by another "Guantlet"
	cout << "\nAnother \"Gauntlet\" of Device calls (Errors expected)" << endl;
	cout << "----------------------------" << endl;

	std::this_thread::sleep_for(std::chrono::seconds(1));

	if (IODev.reset() == -1) {
		cout << "Cannot call reset" << endl;
	}

	std::this_thread::sleep_for(std::chrono::seconds(1));

	if (IODev.start() == -1) {
		cout << "Cannot call start" << endl;
	}

	std::this_thread::sleep_for(std::chrono::seconds(1));

	if (IODev.set_listen() == -1) {
		cout << "Cannot call listen" << endl;
	}

	std::this_thread::sleep_for(std::chrono::seconds(1));

	if(IODev.check_master_fail()) {
		cout << "Master fail in threads" << endl;
		cout << "Stopping server" << endl;
		IODev.stop();
		return 0;
	}

	cout << "No master fail present!" << endl;
	cout << "----------------------------" << endl;
	cout << endl;
	cout << "Printing number of child threads running..." << endl;
	cout << "There are " << IODev.n_running() << " running" << endl;

	cout << endl;
	cout << "\nEnter q to pause device: ";
	while(true) {
		char input = getchar();
		if (input == 'q') {
			int pause_res = IODev.pause();
			if (pause_res == -1)
			   cout << "master fail" << endl;
			break;
		}
	}
	cout << endl;

	if(IODev.resume() == -1) {
		cout << "Failure in third call to IODev.resume()" << endl;
	}
	cout << "Device resumed!" << endl;

	std::this_thread::sleep_for(std::chrono::seconds(2));

	// --- WORK ---	
	// ...
	// ...
	// ...


	// Setting F_master_fail to false to simulate a master fail occuring
	IODev.set_master_fail();

	std::this_thread::sleep_for(std::chrono::seconds(2));
	
	// Program will end here
	if(IODev.check_master_fail()) {
		cout << "Master fail in threads" << endl;
		cout << "Stopping server\n" << endl;
		IODev.stop();
		cout << "End of test...\n" << endl;
		return 0;
	}
	
	cout << "\nEnter q to stop device: ";
	while(true) {
		char input = getchar();
		if (input == 'q') {
			int stop_res = IODev.stop();
			if (stop_res == -1)
			   cout << "master fail" << endl;	
			break;
		}
	}
	return 0;
}
