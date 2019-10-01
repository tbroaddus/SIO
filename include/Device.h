//  File:   Device.h
//  Author:   Tanner Broaddus


#ifndef DEVICE_H
#define DEVICE_H

#include <thread>
#include <string>
#include <vector>
#include <memory>

#include "Options.h"
#include "Worker.h"


namespace ScrybeIO {

class Device {


  public:
    
    friend Worker;

    /*
      Default constructor.
      Cannot handle connections until init() 
      is called  to provide handle function.
    */
    Device();

    /*
      Argument intialized constructor.
      init() does not need to be called since handle 
      function has been provided.
    */
    Device(void(*_handle)(std::string request, int
          client_sock),
        const Options& IO_Options);
  
    // Destructor
    ~Device();
    

    /*
      Provides device with a handle function and Options object to specify 
      behavior. Should only be called if default constructor used.
    */
    int init(void(*_handle)(std::string request, int client_sock), const
        Options& IO_Options);
    
    /*
      Provides Device with a handle function to specify behavior. 
      Default values used for Device.
      Should only be called if default constructor used.
    */
    int init(void(*_handle)(std::string request, int client_sock));

    // Initializes Device listening queue. 
    int set_listen();

    // Starts client handling.
    int start();

    /*
      Stops client handling and clears all connections.
      Must call reset() before restarting server with set_listen() and start()
    */
    int stop();

    /*
      Pauses client handling. 
      All connections are maintained and Device continues 
      to listen for new connections.
    */
    int pause();

    /*
      Resumes client handling after pause().
      Cannot be called after stop().
    */
    int resume();

    /*
      Resets Device's values. Used when either 
      pause() or stop() has been called.
    */
    int reset();

    /*
      Initializes Device's values to the ones provied by the Options object.
      Used when either pause() or stop() has been called.
    */
    int reset(const Options& IO_Options);


    // Returns the amount of worker threads running.
    int n_threads_running() const;
  

  private:

    void (*handle)(std::string request, int client_sock);
    std::vector<std::shared_ptr<Worker>> Worker_vec;
    std::vector<std::thread> thread_vec;
    int epoll_fd;
    int listen_sock;
    int port;
    int thread_count;
    int buffer_size;
    int accept_fail_limit;
    int accept_loop_reset;
    int add_fail_limit;
    int add_loop_reset;
    int max_events;
    int max_listen;
    int timeout;
    bool F_init;
    bool F_running;
    bool F_stop;
    bool F_pause;
    bool F_reset_callable;
    bool F_listening;


}; // Class Device
} // namespace ScrybeIO

#endif
