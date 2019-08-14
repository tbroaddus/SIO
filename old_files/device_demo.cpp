//	File:		device_demo.cpp
//	Author:		Tanner Broaddus
//	Notice:		If using GNU compiler, use -pthread when compliling

#include <iostream>
#include <thread>
#include <chrono>
#include <unistd.h>
#include <stdlib.h>

#include "IO_Device.h"

using std::cout;
using std::endl;
using std::cerr;

/*
   This file is a demonstration of how to use ScrybeIO's Device. If you want to
   test the Device class, use device_test.cpp instead.
*/

/*
   Device procedure:
	1.	Create Options struct and set its member variables to the desired values 
	2.	Create a Device object and pass the address of your handle function and
		your Options object (not by address) into its parameters.
	3.	Call set_listen().
	4.	Call start().
		-	Check if a master fail is present with check_master_fail().
		-	Check the number of threads runnning with n_running().
		-	Pause the device with pause().
		-	Resume the device with resume().
		-	Reset the device with reset().
			*	Call reset() with empty parameters for simple reset.
			*	Call reset() with int value in parameters to reset with new
				port number (value passed in parameters will be defined as the
				new port).
			*	Call reset() with Option object passed in parameters to reset
				with new options.
			*	Start at 1. if any of these reset() functions are called.
	5.	Call stop().
	6.	(Optional) Call reset() and begin again at 1.
*/

//Simple handle function that we will pass into the Device obj
void handle_accept(std::string request, int client_sock) {
	// cout << "From Handle_accept() in test_device.cpp!" << endl;
	// cout << request << endl;
	int sendRes = send(client_sock, request.c_str(), request.size() + 1, 0); 	
	if (sendRes == -1) {
		cout << "Could not send to client!" << endl;
	}
}

int main() {


	

//	1. Create Options struct and set its member variables to the desired values

	ScrybeIO::IODevice::Options Dev_opt;
	
	// Descriptions of Options variables in git wiki
	// Defining struct Options variables 
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
	






//	2.	Create a Device object and pass the address of your handle function and
//		your Options object (not by address) into its parameters.		
	
	ScrybeIO::IODevice::Device IODev(&handle_accept, Dev_opt);






//	3.	Call set_listen()

	if(IODev.set_listen() == -1) {
		cout << "Could not call set_listen()" << endl;
		return 0;
	}
	/*
	   A socket has now been created, binded to a port, and set to listen.
	   Clients are starting to build on the socket's listening queue. The
	   clients won't be accepted until start() is called.
	*/







//	4.	Call start()

	if(IODev.start() == -1) {
		cout << "Could not call start()" << endl;
		return 0;
	}
	/*
	   Now the Device obj is running. Clients are being accepted and their
	   requests are being handled by the handle function (handle_accept() in
	   this example) that we passed when constructing the Device obj. pause()
	   or stop() can be called from here. Work not relating to IO with client
	   can be done on the main thread while the device is running.
	*/

	/*
	   Work 
	   ...
	   ...
	   ...
	   ...
	*/
	std::this_thread::sleep_for(std::chrono::seconds(2));


	// We can check the Device obj's status while doing other work.
	if (IODev.check_master_fail()) {
		cout << "Master fail is present in Device obj" << endl;
		cout << "Stopping Device obj" << endl;
		IODev.stop();

		// We can then call for a hard reset and start the device again...
		IODev.reset();
		if(IODev.set_listen() == -1) {
			cout << "Could not call set_listen()" << endl;
			return 0;
		}
		if(IODev.start() == -1) {
			cout << "Could not call start()" << endl;
			return 0;
		}
		// Device obj should now be running again
	}

	
	// This method also works for checking Device obj's status.
	// The following method of checking the Device obj's status uses the
	// n_running() function to get the number of child threads running in the
	// Device obj.
	int n_threads_running = IODev.n_running();
	// In this case, if more than one thread is not running, we restart the
	// Device obj... if a master fail is present, we want to stop the Device
	// obj and exit the program 
	if (n_threads_running < 3) {
		cout << n_threads_running << " threads running" << endl;
		if (IODev.check_master_fail()) {
			cout << "Master fail is present" << endl;
			cout << "Exiting program..." << endl;
			IODev.stop();
			return 0;
		}
		// No master fail present.
		// We can restart the Device obj from here...
		IODev.stop();
		IODev.reset();
		if(IODev.set_listen() == -1) {
			cout << "Could not call set_listen()" << endl;
			return 0;
		}
		if(IODev.start() == -1) {
			cout << "Could not call start()" << endl;
			return 0;
		}
		// Device obj should now be running again
	}


	/*
	   Work
	   ...
	   ...
	   ...
	   ...
	*/
	std::this_thread::sleep_for(std::chrono::seconds(2));

	
	// You can enter into a loop to stop or pause Device obj at will...
	bool f_restart = false;
	cout << "Enter 's' to \"stop\" device, or 'p' to \"pause\" device: "; 	
	while(true) {
		char input = getchar();
		if (input == 's') {
			int stop_res = IODev.stop();
			if (stop_res == -1) 
				cout << "\nmaster fail present when stopped" << endl;
			cout << "Exiting program" << endl;
			return 0;
		}
		if (input == 'p') {
			int pause_res = IODev.pause();
			// We can define a policy where if there is a master fail present
			// in pause(), the Device obj resets... 
			if (pause_res == -1) {
				f_restart = true;
				cout << "\nmaster fail present when paused" << endl;
				IODev.reset(); // Waiting to restart
			}
			break;	
		}
	}
	cout << endl;

	/* 
	   Work
	   ...
	   ...
	   ...
	*/
	std::this_thread::sleep_for(std::chrono::seconds(2));


	
	// Cannot call resume() directly after reset() so I put a bool flag
	// f_restart to indicate the course of action the main thread will take
	// when "resuming" the Device obj. If a master fail was present when
	// pause() was called, the Device obj called reset() and f_restart was set
	// to true. If a master fail was not present, then resume() can be called.
	// NOTICE: If reset() is called, the Device obj loses all previous clients.
	// Clients would have to reconnect and resend their requests.
	if(f_restart) {
		if(IODev.set_listen() == -1) {
			cout << "Could not call set_listen()" << endl;
			return 0;
		}
		if(IODev.start() == -1) {
			cout << "Could not call start()" << endl;
			return 0;
		}
	} else {
		IODev.resume();
	}
	// Device obj should now be running again.

	std::this_thread::sleep_for(std::chrono::seconds(5));

	

	


//	5.	Call stop().

	if (IODev.stop() == -1)
		cout << "Master fail present in stop()" << endl;

	cout << "End of program... goodbye!\n" << endl;

	return 0;
}
