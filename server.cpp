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

// struct udpCon{
// 	int lastSeqNumber;
// 	int UIN;
// 	int bytesLeft;
// };

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
	if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		perror("Error creating socket");
		exit(-1);
	}
	
	struct sockaddr_in bindAddr;
	memset(&bindAddr, 0, sizeof(bindAddr));
	bindAddr.sin_family = AF_INET;
	bindAddr.sin_port = htons(atoi(port.c_str()));
	bindAddr.sin_addr.s_addr = htonl(0);

	if(bind(sockfd, (struct sockaddr *) &bindAddr, sizeof(bindAddr)) < 0){
		perror("Error binding");
		exit(-1);
	}

	//std::vector<udpCon> cons;

	//check for duplicates, same transmission, and garbled
	while(true){
		struct sockaddr_in recvAddr;
		socklen_t recvAddrLen = sizeof(recvAddr);
		Packet packet;

		memset(&packet, 0, sizeof(packet));

		//cout << "Ready to recv\n";

		int readbytes = recvfrom(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *) &recvAddr, &recvAddrLen);
		if(readbytes < 0){
			perror("Error recieving packet");
		}
		else{

			deNetworkizeHeader(&packet.header);

			//cout << "Received packet\n" << flush;
			int checksum = calcChecksum((void *) &packet, sizeof(packet));

			// int conID = -1;
			// for(int i=0; i<cons.size(); i++){
			// 	if(cons[i].UIN == packet.header.UIN)
			// 		conID = i;
			// }
			// if(conID == -1){
			// 	struct udpCon newCon;
			// 	newCon.lastSeqNumber = packet.header.sequenceNumber;
			// 	newCon.UIN = packet.header.UIN;
			// 	newCon.bytesLeft = packet.header.nTotalBytes;
			// 	cons.push_back();
			// 	conID = cons.size()-1;
			// }

			Packet response = packet;
			response.header.from_IP = packet.header.to_IP;
			response.header.to_IP = packet.header.from_IP;
			response.header.from_Port = packet.header.to_Port;
			response.header.to_Port = packet.header.from_Port;
			response.header.packetType = 2;

			//cout << "checksum: " << checksum << endl;
			//printf("%s\n", packet.data);

			if(checksum == 0)
				response.header.ackNumber = packet.header.sequenceNumber;
			else
				response.header.ackNumber = packet.header.sequenceNumber - 1;

			networkizeHeader(&response.header);

			if(sendto(sockfd, &response, sizeof(response), 0, (struct sockaddr *) &recvAddr, sizeof(recvAddr)) < 0){
				perror("error in sendto");
				exit(-1);
			}
		}

	}

    return EXIT_SUCCESS;
}

