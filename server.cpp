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

int verbose = 0;

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
	int saveFile;

	//sockaddr_in for recieving packets
	struct sockaddr_in recvAddr;
	memset(&recvAddr, 0, sizeof(recvAddr));
	socklen_t recvAddrLen = sizeof(recvAddr);
	Packet packet;

	memset(&packet, 0, sizeof(packet));

	//read in first packet of file
	int readbytes = recvfrom(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *) &recvAddr, &recvAddrLen);
	if(readbytes < 0){
		perror("Error recieving packet");
	}
	else{

		deNetworkizeHeader(&packet.header);

		cout << "Received packet\n" << flush;
		int checksum = calcChecksum((void *) &packet, sizeof(packet));

		//set up response packet
		Packet response = flipAddresses(packet);

		if(checksum != 0 || packet.header.sequenceNumber != expectedSeq){ //bad checksum -- send nak and just treat next packet as new file
			response.header.ackNumber = expectedSeq-1;
			networkizeHeader(&response.header);

			if(verbose)
				cout << "Bad checksum.\n";

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

			//save file if you need to
			ofstream save;
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

					if(verbose)
						cout << "Received packet\n" << flush;

					int checksum = calcChecksum((void *) &packet, sizeof(packet));

					//set up response packet
					response = flipAddresses(packet);

					//bad packet
					if(checksum != 0 || packet.header.UIN != UIN 
							|| packet.header.transactionNumber != transactionNumber
							|| packet.header.sequenceNumber != expectedSeq)
					{ 
						response.header.ackNumber = expectedSeq-1;
						networkizeHeader(&response.header);

						if(verbose)
							cout << "Bad packet.\n";

						if(sendto(sockfd, &response, sizeof(response), 0, (struct sockaddr *) &recvAddr, sizeof(recvAddr)) < 0){
							perror("error in sendto");
							exit(-1);
						}
					}
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

						if(saveFile){
							save.write(packet.data, packet.header.nbytes);
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

	// 	else{ //checksum checks out -- next packets are part of same file
	// 		response.header.ackNumber = packet.header.sequenceNumber;
	// 		int saveFile = packet.header.saveFile;

	// 		networkizeHeader(&response.header);

	// 		//send ack
	// 		if(sendto(sockfd, &response, sizeof(response), 0, (struct sockaddr *) &recvAddr, sizeof(recvAddr)) < 0){
	// 			perror("error in sendto");
	// 			exit(-1);
	// 		}

	// 		bytesLeft = packet.header.nTotalBytes - packet.header.nbytes;
	// 		lastseq = packet.header.sequenceNumber;

	// 		ofstream save;
	// 		if(saveFile){
	// 			save.open(packet.header.filename, ios::out | ios::binary | ios::trunc);
	// 			save.write(packet.data, packet.header.nbytes);
	// 		}

	// 		//loop until rest of packets successfully arrive
	// 		while(bytesLeft > 0){
	// 			Packet subPacket;

	// 			memset(&subPacket, 0, sizeof(subPacket));

	// 			int readbytes = recvfrom(sockfd, &subPacket, sizeof(subPacket), 0, (struct sockaddr *) &recvAddr, &recvAddrLen);
	// 			if(readbytes < 0){
	// 				perror("Error recieving subPacket");
	// 			}

	// 			deNetworkizeHeader(&subPacket.header);
	// 			int subChecksum = calcChecksum(&subPacket, sizeof(subPacket));

	// 			Packet subResponse = flipAddresses(subPacket);

	// 			if(subChecksum != 0){
	// 				subResponse.header.ackNumber = lastseq;
	// 				networkizeHeader(&subResponse.header);

	// 				cout << "Bad checksum... sending NAK.\n";

	// 				if(sendto(sockfd, &subResponse, sizeof(subResponse), 0, (struct sockaddr *) &recvAddr, sizeof(recvAddr)) < 0){
	// 					perror("error in sendto");
	// 					exit(-1);
	// 				}
	// 			}
	// 			else if(subPacket.header.sequenceNumber == lastseq + 1){
	// 				lastseq = subPacket.header.sequenceNumber;
	// 				subResponse.header.ackNumber = lastseq;

	// 				if(saveFile){
	// 					save.write(subPacket.data, subPacket.header.nbytes);
	// 				}

	// 				bytesLeft -= subPacket.header.nbytes;

	// 				networkizeHeader(&subResponse.header);
	// 				if(sendto(sockfd, &subResponse, sizeof(subResponse), 0, (struct sockaddr *) &recvAddr, sizeof(recvAddr)) < 0){
	// 					perror("error in sendto");
	// 					exit(-1);
	// 				}
	// 			}
	// 		}
	// 		if(saveFile){
	// 			save.flush();
	// 			save.close();
	// 			cout << "File saved: " << packet.header.filename << "(" << packet.header.nTotalBytes << " bytes)" << endl;
	// 		}
	// 	}
	}
}

Packet flipAddresses(Packet packet){
	Packet flipped = packet;
	flipped.header.from_IP = packet.header.to_IP;
	flipped.header.to_IP = packet.header.from_IP;
	flipped.header.from_Port = packet.header.to_Port;
	flipped.header.to_Port = packet.header.from_Port;
	flipped.header.packetType = 2;
	return flipped;
}