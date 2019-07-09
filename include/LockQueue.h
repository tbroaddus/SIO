//	File:		LockQueue.h
//	Author:		Tanner Broaddus

#ifndef	LOCKQUEUE_H 
#define LOCKQUEUE_H

#include <queue>
#include <mutex>
#include <string>
#include "ScrybeIOProtocols.h"

#define EMPTY -2

namespace ScrybeIO {

class LockQueue {
	
	public:

		int pop() {
			std::unique_lock<std::mutex> locker(mu);
			if(queue.empty()) {
				return EMPTY;
			}
			int fd = queue.front();
			queue.pop();
			locker.unlock();
			return fd;
		}
		
		void push(const int& fd) {
			std::lock_guard<std::mutex> locker(mu);
			queue.push(fd);
		}

		bool empty() {	
			std::unique_lock<std::mutex> locker(mu);
			bool res = queue.empty();
			locker.unlock();
			return res;	
		}

	private:
	std::queue<int> queue;
	std::mutex mu;
};
}

#endif
