//	File:		IOTest.cpp
//	Author:		Tanner Broaddus
//	NOTICE:		If using GNU compiler, include -pthread when compiling 

#include <iostream>
#include <string>
#include <thread>
#include <unistd.h>

// Might use unordered_map later to simulate request handlers.
// For now, just using one handler that prints string message. 
// #include <unordered_map>
// #include <vector>

#include "ScrybeIO.h"

using std::cout;
using std::endl;
using std::cerr;

void handle_accept(std::string request) {
	cout << "Handle_accept()" << endl;
	cout << request << endl;
}


int main() {

	int listen_fd = ScrybeIO::create_listen_sock(54000, 10);
	if(listen_fd == -1)
		cerr << "Failed to create and bind listening socket\n";
	cout << listen_fd << endl;
	bool done = false;

	// Starting event loop with the following arguments...
	// Pointer to the handle_accept function defined above
	// File descriptor of the listen sock created in create_listen_sock()
	// A value of 10 for up to 10 events returned in epoll_wait();
	// A value of 4096 for buffer size
	// A value of 5000 for a 5000 millisecond (5 Second) wait time for epoll_wait(); 
	// TODO: Test multiple threads
	//std::thread t1(ScrybeIO::event_loop, &handle_accept, listen_fd, 10, 4096, 5000, std::ref(done));
	std::thread t1(ScrybeIO::event_loop, &handle_accept, listen_fd, 10, 4096,
			5000, std::ref(done));
	std::thread t2(ScrybeIO::event_loop, &handle_accept, listen_fd, 10, 4096,
			5000, std::ref(done));

	cout << "Enter 'q' to stop program" << endl;
	char input;
	while(true) {
		input = getchar();
		if (input == 'q') {
			done = true;
			t1.join();
			t2.join();
			break;
		}
	}
	close(listen_fd); // USER RESPONSIBLE FOR CLOSING LISTENING SOCKET 
	cout << "BOOM BABY!" << endl;
	return 0;
}

