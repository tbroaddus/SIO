//  File:   Options.h
//  Author:   Tanner Broaddus

#ifndef OPTIONS_H
#define OPTIONS_H

namespace ScrybeIO
{

class Options
{

public:
  Options();

  ~Options();

  // Set port number.
  void set_port(const int _port);

  // Set number of worker threads.
  void set_tc(const int _tc);

  // Set input buffer size.
  void set_buffer_size(const int _buffer_size);

  /*
    Set number of accept-connection fails tolerated 
    within a specified loop period. A value of 1 would make
    Device accept-fail intolerant. 
  */
  void set_accept_fail_limit(const int _accept_fail_limit);

  // Set accept fail loop period.
  void set_accept_loop_reset(const int _accept_loop_reset);

  /*
    Set number of add-epoll fails tolerated within a specified loop period.
    A value of 1 would make Device add-fail intolerant (recommended).
  */ 
  void set_add_fail_limit(const int _add_fail_limit);

  // Set add fail loop period.
  void set_add_loop_reset(const int _add_loop_reset);

  // Set max number of events returned by epoll in each worker.
  void set_max_events(const int _max_events);

  // Set max number of pending connections on listening queue.
  void set_max_listen(const int _max_listen);

  // Set epoll timeout (in milliseconds).
  void set_timeout(const int _timeout);


  // Getters used by Device functions
  int get_port() const;
  int get_tc() const;
  int get_buffer_size() const;
  int get_accept_fail_limit() const;
  int get_accept_loop_reset() const;
  int get_add_fail_limit() const;
  int get_add_loop_reset() const;
  int get_max_events() const;
  int get_max_listen() const;
  int get_timeout() const;

private:
  int port;
  int tc;
  int buffer_size;
  int accept_fail_limit;
  int accept_loop_reset;
  int add_fail_limit;
  int add_loop_reset;
  int max_events;
  int max_listen;
  int timeout; // In milliseconds

}; // Class Options
} // namespace ScrybeIO

#endif
