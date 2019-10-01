//  File: Worker.h
//  Author: Tanner Broaddus

#ifndef WORKER_H
#define WORKER_H

namespace ScrybeIO {


class Device; // Forward Declaration

// Worker
// Used by Device for client handling.
class Worker {
  
  public:

    Worker();

    ~Worker();

    // Starts handling connections.
    void handle_conns(Device& IODev);

    // Returns running status.
    bool check_running() const;

    // Returns fail status.
    bool check_fail() const;
  
  private:

    int n_accept_fail;
    int n_accept_loop;
    int n_add_fail;
    int n_add_loop;
    bool F_running;
    bool F_fail;


}; // class Worker
} // namespace ScrybeIO

#endif
