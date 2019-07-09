// Author:		Tanner Broaddus
// File:		test.cpp
/*
   I made this small program to test for race conditions and deadlock
   developing the lock queue.
   Be sure to use -pthread extension when compiling.
*/

#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <chrono>

#include "LockQueue.h"
#include "ScrybeIOProtocols.h"

#define DONE 0
#define OK 1

using namespace std::chrono;
using std::cout;
using std::endl;

std::mutex m;

void printfd(int & fd, int & t_num) {
	std::lock_guard<std::mutex> locker(m);
	cout << "FD: " << fd << " From: Thread " << t_num << endl;
}


void listen(ScrybeIO::LockQueue& queue, int& status) {
	for(int i = 0; i < 1000; i++) {
		queue.push(i);
	}
	status = DONE;
}
void handle(ScrybeIO::LockQueue& queue, int& status, int t_num) {
	std::vector<int> fd_vec;
	while(!queue.empty() || status != DONE) {
		int fd = queue.pop();
		if(fd == EMPTY)
			continue;
		fd_vec.push_back(fd);
	}
	int	fd = 100000000;
	printfd(fd, t_num);
	for(int i : fd_vec) {
		printfd(i, t_num);
	}	
}



int main() {

	int status = OK;

	ScrybeIO::LockQueue queue;

	std::thread t1(listen, std::ref(queue),std::ref(status));
	auto start = high_resolution_clock::now();
	std::thread t2(handle, std::ref(queue),std::ref(status), 2);
	std::thread t3(handle, std::ref(queue),std::ref(status), 3);
	std::thread t4(handle, std::ref(queue),std::ref(status), 4);
	//std::thread t5(handle, std::ref(queue),std::ref(status), 5);

	t1.join();
	t2.join();
	t3.join();
	t4.join();
	//t5.join();

	auto stop = high_resolution_clock::now();
	auto duration = duration_cast<microseconds>(stop - start);
	cout << "time to empty queue: " << duration.count() << " microseconds" << endl;
	
	return 0;
}

