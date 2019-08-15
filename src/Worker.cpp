//	File:	Worker.cpp
//	Author:	Tanner Broaddus

#include "Worker.h"

namespace ScrybeIO {


Worker::Worker(const int listen_sock) {

	epoll_fd = epoll_create1(0);
	if (epoll_fd == -1) {
		cerr << "Worker ERR in constructor(): ";
		cerr << "Failure in epoll_create1(0)\n";
		return;
	} 
	struct epoll_event event;
	event.events = EPOLLIN | EPOLLET | EPOLLEXCLUSIVE;
	event.data.fd = listen_sock;
	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_sock, &event) ==
			-1)
	{
		cerr << "Worker ERR in start(): ";
		cerr << "Failed to add listening socket to epoll ";
		cerr << "in handle_conns()\n";
		return;
	}
	F_ready = true;
}



Worker::~Worker() {

	close(epoll_fd);

}



void Worker::handle_conns(Device& IODev) {

	if (!F_ready) {
		cerr << "Worker ERR in handle_conns(): ";
		cerr << "handle_conns called when worker was not ready\n";
		F_fail = true;
		return;
	}
	F_running = true;
	struct epoll_event event, events[IODev.max_events];

	// main event loop
	while(true) {

		if (IODev.F_master_fail == true || IODev.F_stop == true ||
				F_fail == true) {
			F_running = false;
			F_ready = false;
			break;
		}
		if (IODev.F_pause == true) {
			F_running = false;
			break;
		}
		int nfds = epoll_wait(epoll_fd, events, IODev.max_events,
				IODev.timeout);
		if (nfds == -1) {
			cerr << "Worker ERR in handle_conns(): ";
			cerr << "Failure in epoll_wait()\n";
			F_fail = true;
			continue;
		}

		for(int i = 0; i < nfds; i++) {
			if (IODev.F_master_fail == true || IODev.F_stop == true || F_fail
					== true)
				break;
			if (events[i].data.fd == IODev.listen_sock) {
				while (true) {
					if (n_accept_loop == IODev.accept_loop_reset)
					{
						n_accept_fail = 0;
						n_accept_loop = 0;
					}
					if (n_add_loop == IODev.add_loop_reset) {
						n_add_fail = 0;
						n_add_loop = 0;
					}
					struct sockaddr_in client_addr;
					socklen_t client_size = sizeof(client_addr);
					int client_sock = accept(IODev.listen_sock,
							(sockaddr*)&client_addr, &client_size);
					if (client_sock == -1) {
						if ((errno == EAGAIN || errno == EWOULDBLOCK))
							break; // Added all new connections
						else {
							cerr << "Worker ERR in handle_conns(): ";
							cerr << "Failed to accept new client\n";
							n_accept_fail++;
							n_accept_loop++;
							if (n_accept_fail ==
									IODev.accept_fail_limit)
								F_fail = true;
							break; // Could not accept
						}
					}
					int flags = fcntl(client_sock, F_GETFL, 0);
					flags |= O_NONBLOCK;
					fcntl(client_sock, F_SETFL, flags);
					event.events = EPOLLIN | EPOLLET;
					event.data.fd = client_sock;
					if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_sock, &event)
							== -1) {
						cerr << "Worker ERR in handle_conns(): ";
						cerr << "Failure in epoll_ctl to add client fd\n";
						close(client_sock);
						n_add_fail++;
						n_add_loop++;
						if (n_add_fail == IODev.add_fail_limit) 
							F_fail = true;
						break;
					}
					n_accept_loop++;
					n_add_loop++;

					// ----- Testing purposes ----------------------

					char host[NI_MAXHOST];
					char svc[NI_MAXSERV];
					memset(host, 0, NI_MAXHOST);
					memset(svc, 0, NI_MAXSERV);
					int result = getnameinfo((sockaddr*)&client_addr,
							client_size, host, NI_MAXHOST, svc,
							NI_MAXSERV, 0);

					if (result) {
						cout << host << " connected on " << svc <<
							endl;
					}
					else {
						inet_ntop(AF_INET, &client_addr.sin_addr, host,
								NI_MAXHOST);
						cout << host << " connected on " <<
							ntohs(client_addr.sin_port) << endl;
					}

					// ----------------------------------------------

				} // while()
			} // if()
			else {
				if ((events[i].events & EPOLLERR) || (events[i].events &
							EPOLLHUP)) {
					cerr << "Worker ERR in handle_conns(): ";
					cerr << "EPOLLER in Worker\n";
					if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd,
								NULL) == -1) {
						cerr << "Worker ERR in handle_conns(): ";
						cerr << "Could not delete client socket from epoll\n";
						F_fail = true;
						break;
					}
					close(events[i].data.fd);
					continue;
				}
				bool close_fd = false;
				char buff[IODev.buffer_size];
				memset(buff, 0, IODev.buffer_size);

				while (true) {
					int bytes_rec = recv(events[i].data.fd, buff,
							IODev.buffer_size, 0);
					if (bytes_rec < 0) {
						if (errno != EWOULDBLOCK && errno != EAGAIN) {
							cerr << "Worker ERR in handle_conns(): ";
							cerr << "Could not receive message\n";
							close_fd = true;
						}
						break; // No more data to receive
					}
					else if (bytes_rec == 0) {
						cout << "Connection closed" << endl;
						close_fd = true;
						break;
					}
					if (bytes_rec > 0) {
						// cout << bytes_rec << " bytes received" << endl;
						// handle_accept()
						(*IODev.handle)(std::move(std::string(buff)),
								events[i].data.fd);
						continue;
					}
				}
				if (close_fd) {
					if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd,
								NULL) == -1)
					{	
						cerr << "Worker ERR in handle_conns(): ";
						cerr << "Could not delete client socket ";
						cerr << "from epoll\n";
						F_fail = true;
					}
					close(events[i].data.fd);
				}
			} // else()
		} //for()
	} // while()
	if (F_fail == true)
		cerr << "Worker ERR in handle_conns(): Worker thread failed\n";
} // handle_conns()



bool Worker::check_running() const {
	return F_running;
}



bool Worker::check_fail() const {
	return F_fail;
}



bool Worker::ready() const {
	return F_ready;
}


} // namespace ScrybeIO
