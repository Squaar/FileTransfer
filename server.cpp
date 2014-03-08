/*  server.cpp
	
	This program receives a file from a remote client and sends back an
    acknowledgement after the entire file has been received.
    
    Two possible options for closing the connection:
        (1) Send a close command along with the acknowledgement and then close.
        (2) Hold the connection open waiting for additional files / commands.
        The "persistent" field in the header indicates the client's preference.
        
    Written by Matt Dumford January 2014 for CS 450
	mdumfo2@uic.edu
*/


#include <cstdlib>
#include <iostream>
#include <sys/types.h>
#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>
#include <vector>
#include <fstream>
#include <unistd.h>

#include "CS450Header.h"
#include "share.h"

using namespace std;

int main(int argc, char *argv[])
{
    
    // User Input
    
	cout << "Matt Dumford - mdumfo2@uic.edu\n\n";

    /* Check for the following from command-line args, or ask the user:
        Port number to listen to.  Default = 54321.
    */

	string port;
	if(argc > 1)
		port = argv[1];
	else
		port = "54321";


	int sockfd;
	if((sockfd = socket(AF_INET, SOCK_DGRAM, 0) < 0){
		perror("Error creating socket");
		exit(-1);
	}
	
	struct sockaddr_in bindAddr;
	bindAddr.sin_family = AF_INET;
	bindAddr.sin_port = htonl(atoi(port.c_str()));
	bindAddr.sin_addr.s_addr = htonl(0);

	if(bind(sockfd, (struct sockaddr *) &bindAddr, sizeof(bindAddr)) < 0){
		perror("Error binding");
		exit(-1);
	}

	//check for duplicates, same transmission, and garbled
	while(true){
		struct sockaddr_in recvAddr;
		Packet packet;

		memset(&packet, 0, sizeof(packet));

		int readbytes;
		if((readbytes = recvfrom(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *) &recvAddr, sizeof(recvAddr))) < 0){
			perror("Error recieving packet")
			//do something to keep server going
		}

		deNetworkizeHeader(&packet);
		int bytesLeft = packet.header.nTotalBytes; 

		Packet response	
		if(calcChecksum(&packet) != 0){

		}

	}

    return EXIT_SUCCESS;
}

