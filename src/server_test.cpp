//	File:	server_test.cpp
//	Author:	Tanner Broaddus

#include <iostream>
#include <thread>
#include <chrono>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>

// Including Device.h. Also includes Options.h and Worker.h
#include "Device.h"

using namespace std;


// Simple echo function to send message back to client
void handle_accept(std::string request, int client_sock) {
	int sendRes = send(client_sock, request.c_str(), request.size() + 1, 0);
	if (sendRes == -1) 
		cout << "Could not send to client!" << endl;
}


// main() 
int main() {
	
	// Setting Options
	ScrybeIO::Options IO_Options;

	IO_Options.set_port(54000);
	IO_Options.set_tc(4);
	IO_Options.set_buffer_size(1024);
	IO_Options.set_accept_fail_limit(1);
	IO_Options.set_accept_loop_reset(10);
	IO_Options.set_add_fail_limit(1);
	IO_Options.set_add_loop_reset(10);
	IO_Options.set_max_events(10);
	IO_Options.set_max_listen(100);
	IO_Options.set_timeout(1000);

	// Creating Device object
	ScrybeIO::Device IO_Device(&handle_accept, IO_Options);

	// Setting Device to listen (starting listening queue)	
	if(IO_Device.set_listen() == -1) {
		cout << "Could not call set_listen()" << endl;
		return 0;
	}
	
	// Starting Device. Device is now adding connections and handling.
	if(IO_Device.start() == -1) {
		cout << "Could not call start()" << endl;
		return 0;
	}
	

	// -- Work on main thread --
	// --
	// --
	std::this_thread::sleep_for(std::chrono::seconds(2));

	// Printing number of Worker threads currently running (should be 4)
	int n_running = IO_Device.n_threads_running();
	cout << n_running << " threads running!" << endl;

	// Loop to allow user to stop or pause at will
	cout << "Enter 's' to stop or 'p' to pause server: ";
	while(true) {
		char input = getchar();
		if (input == 's') {
			IO_Device.stop();
			break;
		}
		if (input == 'p') {
			int pause_res = IO_Device.pause();
			if (pause_res == -1) {
				cout << "\nFailure in at least one thread when paused" << endl;
				IO_Device.reset();
			}
			break;
		}
	}

		


	// End of progam 
	return 0;
}
