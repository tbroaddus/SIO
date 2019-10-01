#include "gtest/gtest.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <Device.h>

// Simple echo function to send message back to client
void handle_accept(std::string request, int client_sock) {
  int sendRes = send(client_sock, request.c_str(), request.size() + 1, 0);
  if (sendRes == -1) 
    std::cout << "Could not send to client!" << std::endl;
}

TEST(SimpleTest, Device) {

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

  ScrybeIO::Device IO_Device(&handle_accept, IO_Options);

  // Could not call set_listen()"
  ASSERT_NE(IO_Device.set_listen(), -1);
  
  // Starting Device. Device is now adding connections and handling.
  // Could not call start()
  ASSERT_NE(IO_Device.start(), -1);

  // -- Work on main thread --
  std::this_thread::sleep_for(std::chrono::seconds(2));

  //Error: Threads running should be 4
  ASSERT_EQ(IO_Device.n_threads_running(), 4);

  // Failure in atleast one thread when paused
  ASSERT_NE(IO_Device.pause(), -1);

  IO_Device.stop();

}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
