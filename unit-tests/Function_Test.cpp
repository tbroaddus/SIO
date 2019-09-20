//	File:		Function_Test.cpp
//	Author:		Tanner Broaddus


#include "gtest/gtest.h"

#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <Device.h>

using std::cout;
using std::endl;


// Simple echo function to send message back to client
void handle_accept(std::string request, int client_sock) {
	int sendRes = send(client_sock, request.c_str(), request.size() + 1, 0);
	if (sendRes == -1) 
		std::cout << "Could not send to client!" << std::endl; 
}


//				BasicFunctionTest
// ---------------------------------------------------------------------------
/*
	Testing basic start(), pause(), reset(), start(), stop() procedure of
	Device object
*/
TEST(DeviceTest, BasicFunctionTest) {

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

	// Testing set_listen()
	ASSERT_NE(IO_Device.set_listen(), -1) << "Failure in set_listen()";

	// Testing start()	
	ASSERT_NE(IO_Device.start(), -1) << "Failure in start()";
	
	// -- Work on main thread --	
	std::this_thread::sleep_for(std::chrono::seconds(2));

	// Testing n_threads_running()
	ASSERT_EQ(IO_Device.n_threads_running(), 4) << "Failure in "
		"n_threads_running()";

	// Testing pause()
	ASSERT_NE(IO_Device.pause(), -1) << "Worker thread failure detected "
		"in pause()"; 

	// Testing n_threads_running() while Device is not running
	ASSERT_EQ(IO_Device.n_threads_running(), 0) << "Threads reportedly "
		"running while Device is paused";

	// Testing reset() and start up procedure
	ASSERT_NE(IO_Device.reset(), -1) << "reset() was not callable";

	ASSERT_NE(IO_Device.set_listen(), -1) << "Failure in set_listen() "
		"after reset()";

	ASSERT_NE(IO_Device.start(), -1) << "Failure in start() after reset()";

	ASSERT_EQ(IO_Device.n_threads_running(), 4) << "Failure in "
		"n_threads_running()";
	
	// Testing pause() after reset()
	ASSERT_NE(IO_Device.pause(), -1) << "Worker thread failure detected "
		"in pause()";

	// Testing n_threads_running() while Device is not running
	ASSERT_EQ(IO_Device.n_threads_running(), 0) << "Threads reportedly "
		"running while Device is paused";

	// Testing reset(IO_Options) with change in 4 threads to 1 thread
	IO_Options.set_tc(1);

	ASSERT_NE(IO_Device.reset(IO_Options), -1) << "reset(IO_Options) was "
		"not callable";
		
	// Testing set_listen() after reset(IO_Device);
	ASSERT_NE(IO_Device.set_listen(), -1) << "Failure in "
		"set_listen() after reset(IO_Device)";

	// Testing start() after reset(IO_Device)
	ASSERT_NE(IO_Device.start(), -1) << "Failure in start() "
		"after reset(IO_Device)";

	// Testing n_threads_running() 
	ASSERT_EQ(IO_Device.n_threads_running(), 1) << "Failure in "
		"n_threads_running()";

	// Testing stop()
	ASSERT_NE(IO_Device.stop(), -1) << "Failure in stop";

	// Testing n_threads_running() while Device is stopped
	ASSERT_EQ(IO_Device.n_threads_running(), 0) << "Threads reportedly "
		"running while Device is stopped";
}


//				UserErrorTest
// ---------------------------------------------------------------------------
/*
   Testing protection measures designed to prevent inappropriate use of Device
   from the user
*/
TEST(DeviceTest, UserErrorTest) {

	ScrybeIO::Options IO_Options;

	// Setting single-thread device to listen on port 54000
	IO_Options.set_port(54000);
	IO_Options.set_tc(1);
	IO_Options.set_buffer_size(1024);
	Io_Options.set_acept_fail_limit(1);
	IO_Options.set_accept_loop_reset(10);
	IO_Options.set_add_fail_limit(1);
	IO_Options.set_add_loop_reset(10);
	IO_Options.set_max_events(10);
	IO_Options.set_max_listen(100);
	IO_Options.set_timeout(1000);

	ScrybeIO::Device IO_Device(&handle_accept, IO_Options);

	// Testing inappropriate function calls before starting Device

	ASSERT_EQ(IO_Device.start(), -1) << "start() inappropriately called "
		"before set_listen()";
	
	ASSERT_EQ(IO_Device.stop(), -1) << "stop() inappropriately called "
		"before set_listen() and start()";

	ASSERT_EQ(IO_Device.pause(), -1) << "pause() inappropriately called "
		"before set_listen() and start()";

	ASSERT_EQ(IO_Device.resume(), -1) << "resume() inappropriately called "
		"before set_listen(), start(), and pause()";

	ASSERT_EQ(IO_Device.reset(), -1) << "reset() inappropriately called "
		"before pause() or stop()";

	ASSERT_EQ(IO_Device.reset(IO_Options), -1) << "reset(IO_Options) "
		"inappropriately called before pause() or stop()";

	// Starting Device

	ASSERT_NE(IO_Device.set_listen(), -1) << "Could not called set_listen()";

	ASSERT_NE(IO_Device.start(), -1) << "Could not call start()";

	// Testing set_listen(), start(), resume(), reset(), and reset(IO_Options)
	// while running. (All are inappropriate calls while running)

	ASSERT_EQ(IO_Device.set_listen(), -1) << "set_listen() called "
		"while running";

	ASSERT_EQ(IO_Device.start(), -1) << "start() called while "
		"running";

	ASSERT_EQ(IO_Device.resume(), -1) << "resume() called while "
		"running";

	ASSERT_EQ(IO_Device.reset(), -1) << "reset() called while running";

	ASSERT_EQ(IO_Device.reset(IO_Options), -1) << "reset(IO_Options "
		"while running";

	// Pausing Device and testing for inappropriate calls while paused

	ASSERT_NE(IO_Device.pause(), -1) << "Could not call pause()";

	ASSERT_EQ(IO_Device.set_listen(), -1) << "set_listen() called while "
		"paused";	

	ASSERT_EQ(IO_Device.start(), -1) << "start() called while paused";

	ASSERT_EQ(IO_Device.stop(), -1) << "stop() called while "
		"paused";

	ASSERT_EQ(IO_Device.pause(), -1) << "pause() called while paused";

	// Calling reset() and testing innapropriate function calls before starting
	// Device

	ASSERT_NE(IO_Device.reset(), -1) << "Could not call reset()";
	
	ASSERT_EQ(IO_Device.start(), -1) << "start() inappropriately called "
		"before set_listen()";

	ASSERT_EQ(IO_Device.stop(), -1) << "stop() inappropriately called "
		"before set_listen() and start()";

	ASSERT_EQ(IO_Device.pause(), -1) << "pause() inappropriately called "
		"before set_listen() and start()";

	ASSERT_EQ(IO_Device.resume(), -1) << "resume() inappropriately called "
		"before set_listen(), start(), and pause()";

	// Starting Device and testing inppropriate function calls while running

	ASSERT_NE(IO_Device.set_listen(), -1) << "Could not call set_listen()";

	ASSERT_NE(IO_Device.start(), -1) << "Could not call start()";

	ASSERT_EQ(IO_Device.set_listen(), -1) << "set_listen() called "
		"while running";

	ASSERT_EQ(IO_Device.start(), -1) << "start() called while "
		"running";

	ASSERT_EQ(IO_Device.resume(), -1) << "resume() called while "
		"running";

	ASSERT_EQ(IO_Device.reset(), -1) << "reset() called while running";

	ASSERT_EQ(IO_Device.reset(IO_Options), -1) << "reset(IO_Options) "
		"while running";

	// Calling reset(IO_Options) and testing innapropriate function calls before starting
	// Device

	ASSERT_NE(IO_Device.pause(), -1) << "Could not call pause()";

	IO_Options.set_tc(2);

	ASSERT_NE(IO_Device.reset(IO_Options), -1) << "Could not call "
		"reset(IO_Options)";
	
	ASSERT_EQ(IO_Device.start(), -1) << "start() inappropriately called "
		"before set_listen()";

	ASSERT_EQ(IO_Device.stop(), -1) << "stop() inappropriately called "
		"before set_listen() and start()";

	ASSERT_EQ(IO_Device.pause(), -1) << "pause() inappropriately called "
		"before set_listen() and start()";

	ASSERT_EQ(IO_Device.resume(), -1) << "resume() inappropriately called "
		"before set_listen(), start(), and pause()";

	// Starting Device and testing inappropriate function calls while running

	ASSERT_NE(IO_Device.set_listen(), -1) << "Could not call set_listen()";

	ASSERT_NE(IO_Device.start(), -1) << "Could not call start()";

	ASSERT_EQ(IO_Device.set_listen(), -1) << "set_listen() called "
		"while running";

	ASSERT_EQ(IO_Device.start(), -1) << "start() called while "
		"running";

	ASSERT_EQ(IO_Device.resume(), -1) << "resume() called while "
		"running";

	ASSERT_EQ(IO_Device.reset(), -1) << "reset() called while running";

	ASSERT_EQ(IO_Device.reset(IO_Options), -1) << "reset(IO_Options) "
		"while running";

	// Stopping Device and testing inappropriate function calls while stopped

	ASSERT_NE(IO_Device.stop(), -1) << "Could not call stop()";

	ASSERT_EQ(IO_Device.stop(), -1) << "stop() inappropriately called "
		"while stopped";

	ASSERT_EQ(IO_Device.pause(), -1) << "pause() inappropriately called "
		"while stopped";

	ASSERT_EQ(IO_Device.resume(), -1) << "resume() inappropriately called "
		"while stopped";

	ASSERT_EQ(IO_Device.set_listen(), -1) << "set_listen() inappropriately "
		"called while stopped";

	ASSERT_EQ(IO_Device.start(), -1) << "start() inappropriately "
		"called while stopped";
	
	// End of UserErrorTest
}

int main(int argc, char **argv) {
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
