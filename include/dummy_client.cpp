// Code from "Creating a TCP Client in C++ [Linux/Code Blocks]" by Sloan Kelly
// on YouTube.
// Credit to Sloan Kelly for coming up with the following program. I am using
// it to simulate a random client connecting to the server and sending the
// server a message. - Tanner
/*
   We are...
   - Creating a socket
   - Create a hint structure for the server we're connecting with
   - Connecting to the server on the socket.
   - While loop:
		Enter lines of text
		Send to server
		Wait for response
		Desplay response
   - Closing the socket
*/

// Remember: A socket is the reference to the connection that we use within the
// program.

// We only need to worry about creating a port number on the server side, not
// client side.

#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <string>

using namespace std;

int main() {
	
	//Create a socket
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock == -1) {
		return 1;
	}

	// Create a hint structure for the server we're connecting with
	int port = 54000;					//Port we're connecting to on server
	string ipAddress = "127.0.0.1";		//IP address of foreign server

	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(54000);
	inet_pton(AF_INET, ipAddress.c_str(), &hint.sin_addr);

	//Connecting to the server on the socket
	int con_Result = connect(sock, (sockaddr*)&hint, sizeof(hint));
	if(con_Result == -1) {
	   return 1;
	}
	char buf[4096];
	string userInput;

	// Do-While loop
	do {
		//Enter lines of text
		cout << "> ";					//Enter line of text prompt
		getline(cin, userInput);		//Using getline to get the entire line.
		
		//Send to server
		int sendRes = send(sock, userInput.c_str(), userInput.size() + 1, 0);
		if (sendRes == -1) {
			cout << "Could not send to server! Whoops! \r\n";
			continue;
		}

	} while(true);

	// Close the socket
	close(sock);

	return 0;
}

	
