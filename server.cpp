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
#include <iomanip>

#include "CS450Header.h"
#include "share.h"

using namespace std;

int verbose;

void recvFile(int sockfd);
Packet flipAddresses(Packet packet);

int main(int argc, char *argv[])
{
    
    // User Input
    
	cout << "Matt Dumford - mdumfo2@uic.edu\n\n";

	string port;
	if(argc > 1)
		port = argv[1];
	else
		port = "54321";

	verbose = 0;
	if(argc > 2){
		for(int i=2; i<argc; i++){
			if(!strcmp(argv[i], "-v") || !strcmp(argv[i], "-V"))
				verbose = 1;
		}
	}

	//set up socket
	int sockfd; 
	struct addrinfo hints, *res;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM; //udp
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(NULL, port.c_str(), &hints, &res);
	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	bind(sockfd, res->ai_addr, res->ai_addrlen);


	//check for duplicates, same transmission, and garbled
	while(true){
		recvFile(sockfd);
	}

    return EXIT_SUCCESS;
}

void recvFile(int sockfd){
	int bytesLeft;
	int expectedSeq = 1;
	int32_t UIN;
	int32_t transactionNumber;
	int saveFile = 0;
	double percent = 0;
	ofstream save;

	//sockaddr_in for recieving packets
	struct sockaddr_in recvAddr;
	memset(&recvAddr, 0, sizeof(recvAddr));
	socklen_t recvAddrLen = sizeof(recvAddr);
	Packet packet;

	memset(&packet, 0, sizeof(packet));

	//read in first packet of file
	int readbytes = recvfrom(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *) &recvAddr, &recvAddrLen);

	if(verbose)
		cout << "New incoming file.\n" << flush;

	if(readbytes < 0){
		perror("Error recieving packet");
	}
	else{

		deNetworkizeHeader(&packet.header);

		// if(verbose)
		// 	cout << "Received packet\n" << flush;

		int checksum = calcChecksum((void *) &packet, sizeof(Packet));
 
		//set up response packet
		Packet response = flipAddresses(packet);

		if(checksum != 0 || packet.header.sequenceNumber != expectedSeq){ //bad checksum -- send nak and just treat next packet as new file
			response.header.ackNumber = 0;

			networkizeHeader(&response.header);

			if(verbose){
				if(checksum !=0)
					cout << "Bad checksum.\n" << flush;
				else
					cout << "Bad sequence number.\n" << flush;
			}

			if(sendto(sockfd, &response, sizeof(response), 0, (struct sockaddr *) &recvAddr, sizeof(recvAddr)) < 0){
				perror("error in sendto");
				exit(-1);
			}
		}
		else{ //good checksum
			//setup ack
			response.header.ackNumber = expectedSeq;
			networkizeHeader(&response.header);

			//send ack
			if(sendto(sockfd, &response, sizeof(response), 0, (struct sockaddr *) &recvAddr, sizeof(recvAddr)) < 0){
				perror("error in sendto");
				exit(-1);
			}

			bytesLeft = packet.header.nTotalBytes - packet.header.nbytes;
			expectedSeq++;
			UIN = packet.header.UIN;
			transactionNumber = packet.header.transactionNumber;
			saveFile = packet.header.saveFile;
			percent += ((double) packet.header.nbytes / (double) packet.header.nTotalBytes)*100.0;

			if(verbose)
				cout << fixed << setprecision(2) << percent << "%" << endl;

			//save file if you need to
			if(saveFile){
				save.open(packet.header.filename, ios::out | ios::binary | ios::trunc);
				save.write(packet.data, packet.header.nbytes);
			}

			//get the rest of the file
			while(bytesLeft > 0){

				memset(&recvAddr, 0, sizeof(recvAddr));
				memset(&packet, 0, sizeof(packet));

				readbytes = recvfrom(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *) &recvAddr, &recvAddrLen);
				if(readbytes < 0){
					perror("Error recieving packet");
				}
				else{
					deNetworkizeHeader(&packet.header);

					// if(verbose)
					// 	cout << "Received packet\n" << flush;

					checksum = calcChecksum((void *) &packet, sizeof(Packet));

					//set up response packet
					response = flipAddresses(packet);

					//bad packet
					if(checksum != 0 || packet.header.UIN != UIN 
							|| packet.header.transactionNumber != transactionNumber
							|| packet.header.sequenceNumber != expectedSeq)
					{ 
						response.header.ackNumber = expectedSeq-1;

						networkizeHeader(&response.header);

						if(verbose){
							if(checksum != 0)
								cout << "Bad packet.\n" << flush;
							else if(packet.header.UIN != UIN)
								cout << "Bad UIN.\n" << flush;
							else if(packet.header.transactionNumber != transactionNumber)
								cout << "Bad transaction number.\n" << flush;
							else
								cout << "Bad seq: " << packet.header.sequenceNumber << 
										" expected: " << expectedSeq << endl;
						}

						if(sendto(sockfd, &response, sizeof(response), 0, (struct sockaddr *) &recvAddr, sizeof(recvAddr)) < 0){
							perror("error in sendto");
							exit(-1);
						}
					}
					//good packet
					else{
						response.header.ackNumber = expectedSeq;

						networkizeHeader(&response.header);

						//send ack
						if(sendto(sockfd, &response, sizeof(response), 0, (struct sockaddr *) &recvAddr, sizeof(recvAddr)) < 0){
							perror("error in sendto");
							exit(-1);
						}

						bytesLeft -= packet.header.nbytes;
						expectedSeq++;
						percent += ((double) packet.header.nbytes / (double) packet.header.nTotalBytes)*100.0;

						if(verbose)
							cout << fixed << setprecision(2) << percent << "%" << endl;

						if(saveFile){
							save.write(packet.data, packet.header.nbytes);
						}
					}
				}
			}	
		}
	}
	if(saveFile){
		save.flush();
		save.close();
		cout << "File saved: " << packet.header.filename << "(" << packet.header.nTotalBytes << " bytes)" << endl;
	}

}

Packet flipAddresses(Packet packet){
	Packet flipped = packet;
	flipped.header.from_IP = packet.header.to_IP;
	flipped.header.to_IP = packet.header.from_IP;
	flipped.header.from_Port = packet.header.to_Port;
	flipped.header.to_Port = packet.header.from_Port;
	flipped.header.trueToIP = packet.header.trueFromIP;
	flipped.header.trueFromIP = packet.header.trueToIP;
	flipped.header.packetType = 2;
	return flipped;
}