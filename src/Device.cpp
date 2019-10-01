//  File: Device.cpp
//  Author: Tanner Broaddus


#include <iostream>
#include <cstdio>

#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <netdb.h>
#include <errno.h>

#include "Device.h"

using std::cout;
using std::endl;
using std::cerr;
using namespace ScrybeIO;




Device::Device() {
  handle = nullptr;
  epoll_fd = -1;
  listen_sock = -1;
  port = 54000;
  thread_count = 1;
  buffer_size = 1024;
  accept_fail_limit = 1;
  accept_loop_reset = 10;
  add_fail_limit = 1;
  add_loop_reset = 10;
  max_events = 10;
  max_listen = 100;
  timeout = 1000;
  F_init = false;
  F_running = false;
  F_stop = false;
  F_pause = false;
  F_reset_callable = false;
  F_listening = false;
}



Device::Device(void(*_handle)(std::string request, int client_sock), const
    Options& IO_Options) {
  handle = _handle;
  epoll_fd = -1;
  listen_sock = -1;
  port = IO_Options.get_port();
  thread_count = IO_Options.get_tc();
  buffer_size = IO_Options.get_buffer_size();
  accept_fail_limit = IO_Options.get_accept_fail_limit();
  accept_loop_reset = IO_Options.get_accept_loop_reset();
  add_fail_limit = IO_Options.get_add_fail_limit();
  add_loop_reset = IO_Options.get_add_loop_reset();
  max_events = IO_Options.get_max_events();
  max_listen = IO_Options.get_max_listen();
  timeout = IO_Options.get_timeout();
  F_init = true;
  F_running = false;
  F_stop = false;
  F_pause = false;
  F_reset_callable = false;
  F_listening = false;
}



Device::~Device() {
  if(F_running == true) {
    this->stop();
  }
}



int Device::init(void(*_handle)(std::string request, int client_sock), const
    Options& IO_Options) {
  if (F_init == true) {
    cerr << "Device ERR in init(): ";
    cerr << "Device already initialized after default constructor\n";
    return -1;
  }
  handle = _handle;
  port = IO_Options.get_port();
  thread_count = IO_Options.get_tc();
  buffer_size = IO_Options.get_buffer_size();
  accept_fail_limit = IO_Options.get_accept_fail_limit();
  accept_loop_reset = IO_Options.get_accept_loop_reset();
  add_fail_limit = IO_Options.get_add_fail_limit();
  add_loop_reset = IO_Options.get_add_loop_reset();
  max_events = IO_Options.get_max_events();
  max_listen = IO_Options.get_max_listen();
  timeout = IO_Options.get_timeout();
  F_init = true;
  return 0;
}



int Device::init(void(*_handle)(std::string request, int client_sock)) {
  if (F_init == true) {
    cerr << "Device ERR in init(): ";
    cerr << "Device already initialized after default constructor\n";
    return -1;
  }
  handle = _handle;
  F_init = true;
  return 0;
}



int Device::set_listen() {
  if (F_init == false) { 
    cerr << "Device ERR in set_listen(): ";
    cerr << "Device has not been initialized after default constructor\n";
    return -1;
  }
  if (F_running == true) {
    cerr << "Device ERR in set_listen(): ";
    cerr << "Device already listening and running\n";
    return -1;
  }
  if (F_stop == true || F_pause == true) {
    cerr << "Device ERR in set_listen(): ";
    cerr << "Must call reset() before set_listen()\n";
    return -1;
  }
  F_reset_callable = false;
  listen_sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
  if (listen_sock == -1) {
    cerr << "Device ERR in set_listen(): ";
    cerr << "Cannot create listening socket\n";
    F_reset_callable = true;
    return -1;
  }
  int set_sock = 1;
  // Allows bind() without error after reset() call
  if (setsockopt(listen_sock, SOL_SOCKET, SO_REUSEPORT, &set_sock,
        sizeof(set_sock)) == -1) {
    cerr << "Device ERR in set_listen(): ";
    cerr << "Failure in setsockopt()\n";
    close(listen_sock);
    F_reset_callable = true;
    return -1;
  }
  sockaddr_in listen_addr;
  listen_addr.sin_family = AF_INET;
  listen_addr.sin_port = htons(port);
  inet_pton(AF_INET, "0.0.0.0", &listen_addr.sin_addr);
  if (bind(listen_sock, (sockaddr*)&listen_addr, sizeof(listen_addr)) == -1)
  {
    cerr << "Device ERR in set_listen(): ";
    cerr << "Failure to bind listening socket\n";
    close(listen_sock);
    F_reset_callable = true;
    return -1;
  }
  if (listen(listen_sock, max_listen) == -1) {
    cerr << "Device ERR in set_listen(): ";
    cerr << "Failed to set socket to listen\n";
    close(listen_sock);
    F_reset_callable = true;
    return -1;
  }
  F_listening = true;

  return 0;
} // set_listen()



int Device::start() {
  if (F_listening == false) {
    cerr << "Device ERR in start(): ";
    cerr << "Must call set_listen() before start()\n";
    return -1;
  }
  if (F_running == true) {
    cerr << "Device ERR in start(): Device already running\n";
    return -1;
  }
  if (F_pause == true) {
    cerr << "Device ERR in start(): ";
    cerr << "Must call reset(), set_listen() before start()\n";
    return -1;
  }
  if (thread_count < 1) {
    cerr << "Device ERR in start(): ";
    cerr << "thread_count must be at least 1\n";
    close(listen_sock);
    F_reset_callable = true;
    return -1;
  }
  epoll_fd = epoll_create1(0);
  if (epoll_fd == -1) {
    cerr << "Device ERR in start(): ";
    cerr << "Failure in epoll_create1(0)\n";
    close(listen_sock);
    F_listening = false;
    F_reset_callable = true;
    return -1;
  }
  struct epoll_event event;
  event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
  event.data.fd = listen_sock;
  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_sock, &event) == -1) {
    cerr << "Device ERR in start(): ";
    cerr << "Failed to add listening socket to epoll\n";
    close(listen_sock);
    F_listening = false;
    F_reset_callable = true;
    return -1;
  }
  for(int i = 0; i < thread_count; i++) {
    Worker_vec.push_back(std::make_shared<Worker>());
  }
  for(std::shared_ptr<Worker> Worker_ptr : Worker_vec) {
    thread_vec.push_back(std::move(std::thread(&Worker::handle_conns,
            Worker_ptr, std::ref(*this))));
  }
  F_stop = false;
  F_running = true;

  return 0;
} // start()



int Device::stop() {
  if (F_running == false) {
    cerr << "Device ERR in stop(): stop() called while not running\n";
    return -1;
  }
  F_stop = true;
  for(std::thread& Worker_thread : thread_vec) {
    Worker_thread.join();
  }
  thread_vec.clear();
  close(listen_sock);
  F_listening = false;
  F_running = false;
  F_reset_callable = true;

  return 0;
} // stop()



int Device::pause() {
  if (F_running == false) {
    cerr << "Device ERR in pause(): ";
    cerr << "pause() called while device not running\n";
    return -1;
  }
  F_pause = true;
  F_running = false;
  for(std::thread& Worker_thread : thread_vec) {
    Worker_thread.join();
  }
  thread_vec.clear();
  F_reset_callable = true;
  int n_Workers_failed = 0;
  for(std::shared_ptr<Worker> Worker_ptr : Worker_vec) {
    if (Worker_ptr->check_fail()) 
      n_Workers_failed++;
  }
  if (n_Workers_failed > 0) {
    cerr << "Device ERR in pause(): ";
    cerr << n_Workers_failed << " Worker threads failed\n";
    close(listen_sock);
    F_listening = false;
    F_pause = false; // We do not want the ability to call resume() if all
             // Worker threads failed
    return -1;  
  }
  
  return 0;
} // pause() 



int Device::resume() {
  if (F_pause == false) {
    cerr << "Device ERR in resume(): ";
    cerr << "Device could not be resumed\n";
    return -1;
  }
  F_reset_callable = false;
  F_pause = false;
  for(std::shared_ptr<Worker> Worker_ptr : Worker_vec) {
    thread_vec.push_back(std::move(std::thread(&Worker::handle_conns,
            Worker_ptr, std::ref(*this))));
  }
  F_running = true;
  
  return 0;
} // resume() 



int Device::reset() {
  if (F_reset_callable == false) {
    cerr << "Device ERR in reset(): reset() not callable\n";
    return -1;
  }
  if (F_listening == true) {
    close(listen_sock);
    F_listening = false;
  }
  close(epoll_fd);
  Worker_vec.clear();
  F_stop = false;
  F_pause = false;

  return 0;
} // reset() 



int Device::reset(const Options& IO_Options) {
  if (F_reset_callable == false) {
    cerr << "Device ERR in reset(): reset() not callable\n";
    return -1;
  }
  port = IO_Options.get_port();
  thread_count = IO_Options.get_tc();
  buffer_size = IO_Options.get_buffer_size();
  accept_fail_limit = IO_Options.get_accept_fail_limit();
  accept_loop_reset = IO_Options.get_accept_loop_reset();
  add_fail_limit = IO_Options.get_add_fail_limit();
  add_loop_reset = IO_Options.get_add_loop_reset();
  max_events = IO_Options.get_max_events();
  max_listen = IO_Options.get_max_listen();
  timeout = IO_Options.get_timeout();
  if (F_listening == true) {
    close(listen_sock);
    F_listening = false;
  }
  close(epoll_fd);
  Worker_vec.clear();
  F_stop = false;
  F_pause = false;

  return 0;
} // reset(const Options&)



int Device::n_threads_running() const {
  if (F_running == false)
    return 0;
  int n_running = 0;
  for (std::shared_ptr<Worker> Worker_ptr : Worker_vec) {
    if (Worker_ptr->check_running())
      n_running++;
  }
  
  return n_running;
} // n_running() 


