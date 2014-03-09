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


	// int sockfd;
	// if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
	// 	perror("Error creating socket");
	// 	exit(-1);
	// }
	
	// struct sockaddr_in bindAddr;
	// memset(&bindAddr, 0, sizeof(bindAddr));
	// bindAddr.sin_family = AF_INET;
	// bindAddr.sin_port = htons(atoi(port.c_str()));
	// bindAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	// if(bind(sockfd, (struct sockaddr *) &bindAddr, sizeof(bindAddr)) < 0){
	// 	perror("Error binding");
	// 	exit(-1);
	// }

	int sockfd; 
	struct addrinfo hints, *res;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;
	getaddrinfo(NULL, port.c_str(), &hints, &res);
	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	bind(sockfd, res->ai_addr, res->ai_addrlen);


	//check for duplicates, same transmission, and garbled
	while(true){
		int bytesLeft;
		int lastseq;

		struct sockaddr_in recvAddr;
		socklen_t recvAddrLen = sizeof(recvAddr);
		Packet packet;

		memset(&packet, 0, sizeof(packet));

		int readbytes = recvfrom(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *) &recvAddr, &recvAddrLen);
		if(readbytes < 0){
			perror("Error recieving packet");
		}
		else{

			deNetworkizeHeader(&packet.header);

			cout << "Received packet\n" << flush;
			int checksum = calcChecksum((void *) &packet, sizeof(packet));

			Packet response = packet;
			response.header.from_IP = packet.header.to_IP;
			response.header.to_IP = packet.header.from_IP;
			response.header.from_Port = packet.header.to_Port;
			response.header.to_Port = packet.header.from_Port;
			response.header.packetType = 2;

			//cout << "checksum: " << checksum << endl;
			//printf("%s\n", packet.data);

			if(checksum != 0){ //bad checksum -- just treat next packet as new file
				response.header.ackNumber = packet.header.sequenceNumber - 1;
				networkizeHeader(&response.header);

				if(sendto(sockfd, &response, sizeof(response), 0, (struct sockaddr *) &recvAddr, sizeof(recvAddr)) < 0){
					perror("error in sendto");
					exit(-1);
				}
			}


			else{ //checksum checks out -- next packets are part of same file
				response.header.ackNumber = packet.header.sequenceNumber;
				int saveFile = packet.header.saveFile;

				networkizeHeader(&response.header);

				if(sendto(sockfd, &response, sizeof(response), 0, (struct sockaddr *) &recvAddr, sizeof(recvAddr)) < 0){
					perror("error in sendto");
					exit(-1);
				}

				bytesLeft = packet.header.nTotalBytes - packet.header.nbytes;
				lastseq = packet.header.sequenceNumber;

				ofstream save;
				if(saveFile){
					save.open(packet.header.filename, ios::out | ios::binary | ios::trunc);
					save.write(packet.data, packet.header.nbytes);
				}

				while(bytesLeft > 0){
					Packet subPacket;

					memset(&subPacket, 0, sizeof(subPacket));

					int readbytes = recvfrom(sockfd, &subPacket, sizeof(subPacket), 0, (struct sockaddr *) &recvAddr, &recvAddrLen);
					if(readbytes < 0){
						perror("Error recieving subPacket");
					}

					deNetworkizeHeader(&subPacket.header);
					int subChecksum = calcChecksum(&subPacket, sizeof(subPacket));

					Packet subResponse = subPacket;
					subResponse.header.from_IP = subPacket.header.to_IP;
					subResponse.header.to_IP = subPacket.header.from_IP;
					subResponse.header.from_Port = subPacket.header.to_Port;
					subResponse.header.to_Port = subPacket.header.from_Port;
					subResponse.header.packetType = 2;

					if(subChecksum != 0){
						subResponse.header.ackNumber = lastseq;
						networkizeHeader(&subResponse.header);

						if(sendto(sockfd, &subResponse, sizeof(subResponse), 0, (struct sockaddr *) &recvAddr, sizeof(recvAddr)) < 0){
							perror("error in sendto");
							exit(-1);
						}
					}
					else{
						lastseq = subPacket.header.sequenceNumber;
						subResponse.header.ackNumber = lastseq;

						if(saveFile){
							save.write(subPacket.data, subPacket.header.nbytes);
						}

						bytesLeft -= subPacket.header.nbytes;

						networkizeHeader(&subResponse.header);
						if(sendto(sockfd, &subResponse, sizeof(subResponse), 0, (struct sockaddr *) &recvAddr, sizeof(recvAddr)) < 0){
							perror("error in sendto");
							exit(-1);
						}
					}
				}
				if(saveFile){
					save.flush();
					save.close();
					cout << "File saved: " << packet.header.filename << "(" << packet.header.nTotalBytes << " bytes)" << endl;
				}

			}



		}

	}

    return EXIT_SUCCESS;
}

