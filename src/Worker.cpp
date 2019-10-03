//  File: Worker.cpp
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

#include "Worker.h"
#include "Device.h"

using std::cerr;
using std::cout;
using std::endl;
using namespace ScrybeIO;

Worker::Worker()
{
  n_accept_fail = 0;
  n_accept_loop = 0;
  n_add_fail = 0;
  n_add_loop = 0;
  F_running = false;
  F_fail = false;
}

Worker::~Worker() {}

void Worker::handle_conns(Device &IODev)
{
  F_running = true;
  struct epoll_event event, events[IODev.max_events];
  // main event loop
  while (true)
  {
    if (IODev.F_stop == true || IODev.F_pause == true || F_fail == true)
    {
      F_running = false;
      break;
    }
    int nfds = epoll_wait(IODev.epoll_fd, events, IODev.max_events,
                          IODev.timeout);
    if (nfds == -1)
    {
      cerr << "Worker ERR in handle_conns(): ";
      cerr << "Failure in epoll_wait()\n";
      F_fail = true;
      continue;
    }

    for (int i = 0; i < nfds; i++)
    {
      if (IODev.F_stop == true || F_fail == true)
        break;
      if (events[i].data.fd == IODev.listen_sock)
      {
        while (true)
        {
          if (n_accept_loop == IODev.accept_loop_reset)
          {
            n_accept_fail = 0;
            n_accept_loop = 0;
          }
          if (n_add_loop == IODev.add_loop_reset)
          {
            n_add_fail = 0;
            n_add_loop = 0;
          }
          struct sockaddr_in client_addr;
          socklen_t client_size = sizeof(client_addr);
          int client_sock = accept(IODev.listen_sock,
                                   (sockaddr *)&client_addr, &client_size);
          if (client_sock == -1)
          {
            if ((errno == EAGAIN || errno == EWOULDBLOCK))
              break; // Added all new connections
            else
            {
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
          event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
          event.data.fd = client_sock;
          if (epoll_ctl(IODev.epoll_fd, EPOLL_CTL_ADD, client_sock, &event) == -1)
          {
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
            cout << "DEVICE> " << host << " connected on " << svc <<
              endl;
          }
          else {
            inet_ntop(AF_INET, &client_addr.sin_addr, host,
                NI_MAXHOST);
            cout << "DEVICE> " << host << " connected on " <<
              ntohs(client_addr.sin_port) << endl;
          }

          // ----------------------------------------------
        } // while()
        epoll_event event;
        event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
        event.data.fd = IODev.listen_sock;
        if (epoll_ctl(IODev.epoll_fd, EPOLL_CTL_MOD, IODev.listen_sock,
                      &event) == -1)
        {
          cerr << "Device ERR in handle_conns(): ";
          cerr << "Failure in epoll_ctl \n";
          F_fail = true;
        }
      } // if()
      else
      {
        if ((events[i].events & EPOLLERR) || (events[i].events &
                                              EPOLLHUP))
        {
          cerr << "Worker ERR in handle_conns(): ";
          cerr << "EPOLLER in Worker\n";
          if (epoll_ctl(IODev.epoll_fd, EPOLL_CTL_DEL, events[i].data.fd,
                        NULL) == -1)
          {
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
        while (true)
        {
          int bytes_rec = recv(events[i].data.fd, buff,
                               IODev.buffer_size, 0);
          if (bytes_rec < 0)
          {
            if (errno != EWOULDBLOCK && errno != EAGAIN)
            {
              cerr << "Worker ERR in handle_conns(): ";
              cerr << "Could not receive message\n";
              close_fd = true;
            }
            break; // No more data to receive
          }
          else if (bytes_rec == 0)
          {
            cout << "DEVICE> Connection closed" << endl;
            close_fd = true;
            break;
          }
          else
          {
            // cout << bytes_rec << " bytes received" << endl;
            // handle_accept()
            (*IODev.handle)(std::move(std::string(buff)),
                            events[i].data.fd);
            continue;
          }
        }
        if (close_fd)
        {
          if (epoll_ctl(IODev.epoll_fd, EPOLL_CTL_DEL, events[i].data.fd,
                        NULL) == -1)
          {
            cerr << "Worker ERR in handle_conns(): ";
            cerr << "Could not delete client socket ";
            cerr << "from epoll\n";
            F_fail = true;
          }
          close(events[i].data.fd);
        }
        else
        {
          epoll_event event;
          event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
          event.data.fd = events[i].data.fd;
          if (epoll_ctl(IODev.epoll_fd, EPOLL_CTL_MOD,
                        events[i].data.fd, &event) == -1)
          {
            cerr << "Worker ERR in handle_conns(): ";
            cerr << "Failure in epoll_ctl \n";
            F_fail = true;
          }
        }
      } // else()
    }   //for()
  }     // while()
  if (F_fail == true)
    cerr << "Worker ERR in handle_conns(): Worker thread failed\n";
} // handle_conns()

bool Worker::check_running() const
{
  return F_running;
}

bool Worker::check_fail() const
{
  return F_fail;
}
