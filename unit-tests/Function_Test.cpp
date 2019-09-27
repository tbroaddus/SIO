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


// Testing init functions

TEST(InitTest, InitHandleOptions) {
	ScrybeIO::Options IO_Options;
	ScrybeIO::Device IO_Device;
	ASSERT_NE(IO_Device.init(&handle_accept, IO_Options), -1);
}

TEST(InitTest, InitHandle) {
	ScrybeIO::Device IO_Device;
	ASSERT_NE(IO_Device.init(&handle_accept), -1);
}


// Test fixture for individual function test
class DeviceTest : public ::testing::Test {
	protected:
		void SetUp() override {
			IO_Options.set_port(54000);
			IO_Options.set_tc(1);
			IO_Options.set_buffer_size(1024);
			IO_Options.set_accept_fail_limit(1);
			IO_Options.set_accept_loop_reset(10);
			IO_Options.set_add_fail_limit(1);
			IO_Options.set_add_loop_reset(10);
			IO_Options.set_max_events(10);
			IO_Options.set_max_listen(100);
			IO_Options.set_timeout(1000);

			IO_Device.init(&handle_accept, IO_Options);
		}

		ScrybeIO::Options IO_Options;
		ScrybeIO::Device IO_Device;
};

// ---- Function Tests ----

TEST_F(DeviceTest, SetListen) {
	ASSERT_NE(IO_Device.set_listen(), -1);
}

TEST_F(DeviceTest, Start) {
	IO_Device.set_listen();
	ASSERT_NE(IO_Device.start(), -1);
}

TEST_F(DeviceTest, Stop) {
	IO_Device.set_listen();
	IO_Device.start();
	ASSERT_NE(IO_Device.stop(), -1);
}

TEST_F(DeviceTest, Pause) {
	IO_Device.set_listen();
	IO_Device.start();
	ASSERT_NE(IO_Device.pause(), -1);
}

TEST_F(DeviceTest, Resume) {
	IO_Device.set_listen();
	IO_Device.start();
	IO_Device.pause();
	ASSERT_NE(IO_Device.resume(), -1);
}

TEST_F(DeviceTest, StopResetNoArgs) {
	IO_Device.set_listen();
	IO_Device.start();
	IO_Device.stop();
    ASSERT_NE(IO_Device.reset(), -1);
}

TEST_F(DeviceTest, PauseResetNoArgs) {
	IO_Device.set_listen();
	IO_Device.start();
	IO_Device.pause();
	ASSERT_NE(IO_Device.reset(), -1);
}

TEST_F(DeviceTest, StopResetArgs) {
	IO_Device.set_listen();
	IO_Device.start();
	IO_Device.stop();
    ASSERT_NE(IO_Device.reset(IO_Options), -1);
}

TEST_F(DeviceTest, PauseResetArgs) {
	IO_Device.set_listen();
	IO_Device.start();
	IO_Device.pause();
    ASSERT_NE(IO_Device.reset(IO_Options), -1);
}

TEST_F(DeviceTest, NThreadsRunning) {
	IO_Device.set_listen();
	IO_Device.start();
	std::this_thread::sleep_for(std::chrono::seconds(1));
	ASSERT_EQ(IO_Device.n_threads_running(), 1);
	IO_Device.stop();
	IO_Options.set_tc(3);
	IO_Device.reset(IO_Options);
	IO_Device.set_listen();
	IO_Device.start();
	std::this_thread::sleep_for(std::chrono::seconds(1));
	ASSERT_EQ(IO_Device.n_threads_running(), 3);
}

int main(int argc, char **argv) {
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
