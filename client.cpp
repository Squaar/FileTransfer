/*  client.cpp

	this program transfers a file to a remote server, possibly through an
    intermediary relay.  The server will respond with an acknowledgement.
    
    This program then calculates the round-trip time from the start of
    transmission to receipt of the acknowledgement, and reports that along
    with the average overall transmission rate.
    
    Written by Matt Dumford for CS 450 HW1 January 2014
	mdumfo2@uic.edu
*/

#include <cstdlib>
#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#include "CS450Header.h"
#include "share.h"

using namespace std;

int main(int argc, char *argv[])
{
    // User Input

    cout << "Matt Dumford - mdumfo2@uic.edu\n\n";
    
    //CS450VA - 54.84.21.227
    //server port - 54323
    //relay port - 54322

	string server;
	if(argc > 1)
		server = argv[1];	
	else
		server = "127.0.0.1";

	string port;
	if(argc > 2)
		port = argv[2];
	else
		port = "54321";

	string relay;
	if(argc > 3)
		relay = argv[3];
	else
		relay = "none";

	string garbleChance;
	if(argc > 4)
		garbleChance = argv[4];
	else
		garbleChance = "0";

	string myPort = "54329";

	string relayPort = "54322";

	int persistent = 0;
	int saveFile = 0;
	int verbose = 0;
	if(argc > 5){
		for(int i=5; i<argc; i++){
			if(!strcmp(argv[i], "-p") || !strcmp(argv[i], "-P"))
				persistent = 1;
			if(!strcmp(argv[i], "-s") || !strcmp(argv[i], "-S"))
				saveFile = 1;
			if(!strcmp(argv[i], "-v") || !strcmp(argv[i], "-V"))
				verbose = 1;
		}
	}

	int useRelay = 0;
	if(relay.compare("none"))
		useRelay = 1;

	//get all needed IPs

	char myHostname[128];
	gethostname(myHostname, sizeof(myHostname));
	struct hostent *myHe = gethostbyname(myHostname);
	u_int32_t myIP = ((struct in_addr **) myHe->h_addr_list)[0]->s_addr;

	struct hostent *toHe = gethostbyname(server.c_str());
	u_int32_t toIP = ((struct in_addr **) toHe->h_addr_list)[0]->s_addr;

	struct hostent *relayHe;
	//u_int32_t relayIP;
	if(useRelay){
		relayHe = gethostbyname(relay.c_str());
		//relayIP = ((struct in_addr **) relayHe->h_addr_list)[0]->s_addr;
		// cout << "relay IP: " << relayIP << endl;
	}


    int transactionNumber = 1;
	do{

		//memory map the file
		cout << "Enter a file to send, or exit to exit.\n>";
		string filePath;
		cin >> filePath;

		if(filePath.compare("exit") == 0)
			exit(0);

		int fd = open(filePath.c_str(), O_RDONLY);
		if(fd == -1){
			perror("Error opening file");
			exit(-1);
		}

		struct stat stats;
		if(fstat(fd, &stats) < 0){
			perror("Error in fstat");
			exit(-1);
		}

		size_t fileSize = stats.st_size;

		char *file = (char *) mmap(NULL, fileSize, PROT_READ, MAP_SHARED, fd, 0);
		if(file == MAP_FAILED){
			perror("Error in mmap");
			exit(-1);
		}


		//set up the socket
		struct sockaddr_in sendAddr;
		memset(&sendAddr, 0, sizeof(sendAddr));

		int sockfd; 
		struct addrinfo hints, *res;

		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_DGRAM;
		hints.ai_flags = AI_PASSIVE;

		if(useRelay){
			sendAddr.sin_port = htons(atoi(relayPort.c_str()));
			memcpy((void *) &sendAddr.sin_addr, relayHe->h_addr_list[0], relayHe->h_length);
		}
		else{
			sendAddr.sin_port = htons(atoi(port.c_str()));
			memcpy((void *) &sendAddr.sin_addr, toHe->h_addr_list[0], toHe->h_length);
		}
		getaddrinfo(NULL, myPort.c_str(), &hints, &res);
		sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		bind(sockfd, res->ai_addr, res->ai_addrlen);

		time_t timer = time(NULL);
	    
		int bytesLeft = fileSize;
		int sequenceNumber = 0;
		while(bytesLeft > 0){

			int bytesToSend = (bytesLeft < BLOCKSIZE) ? bytesLeft : BLOCKSIZE;

			Packet packet;
			memset(&packet, 0, sizeof(packet));

			//fill in packet fields
			packet.header.version = 6;
			packet.header.UIN = 675005893;
			packet.header.transactionNumber = transactionNumber;
			packet.header.sequenceNumber = sequenceNumber;
			packet.header.from_IP = myIP;
			packet.header.to_IP = toIP;
			packet.header.trueFromIP = myIP;
			packet.header.trueToIP = myIP;
			packet.header.nbytes = bytesToSend;
			packet.header.nTotalBytes = fileSize;
			strcpy(packet.header.filename, filePath.c_str());
			packet.header.from_Port = atoi(myPort.c_str());
			packet.header.to_Port = atoi(port.c_str());
			//put checksum in after everything else
			packet.header.HW_number = 2;
			packet.header.packetType = 1;
			packet.header.saveFile = saveFile;
			packet.header.dropChance = 0;
			packet.header.dupeChance = 0;
			//packet.header.garbleChance = atoi(garbleChance.c_str());
			packet.header.garbleChance = 100;
			packet.header.protocol = 22;

			const char *ACCC = "mdumfo2";
       		memcpy(&packet.header.ACCC, ACCC, strlen(ACCC));

       		//copy data into packet
			memcpy(&packet.data, file + sequenceNumber, bytesToSend);

			//create checksum and put in header
			packet.header.checksum = calcChecksum((void *) &packet, sizeof(packet));

			//make sure checksum worked
			uint16_t newcheck = calcChecksum((void *) &packet, sizeof(packet));
			if(newcheck != 0){
				cout << "Bad checksum... Exiting.\n";
				exit(-1);
			}

			// cout << "checksum: " << calcChecksum((void *) &packet, sizeof(packet)) << endl;

			//STEPS:
			//	1-send packet
			//	2-wait for ACK
			//	3a-ACK is not corrupt and ACK==sequenceNumber, send next packet
			//	3b-ACK is corrupt or ACK!=sequenceNumber, resend current packet

			//send packet
			networkizeHeader(&packet.header);
			if(sendto(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *) &sendAddr, sizeof(sendAddr)) < 0){
				perror("error in sendto");
				exit(-1);
			}

			//cout << "packet sent\n" << flush;

			bool ready; //is server ready for next packet?
			do{

				//get response from server
				struct sockaddr_in responseAddr;
				memset(&responseAddr, 0, sizeof(responseAddr));
				Packet response;
				socklen_t responseAddrLen = sizeof(responseAddr);
				int readBytes = recvfrom(sockfd, &response, sizeof(response), 0, (struct sockaddr *) &responseAddr, &responseAddrLen);
				if(readBytes < 0){
					perror("Error in recvfrom");
					exit(-1);
				}
				deNetworkizeHeader(&response.header);
				ready = (response.header.ackNumber == sequenceNumber);

				//cout << "packet recieved\n" << flush;
				//cout << response.header.ackNumber << endl;
				if(verbose)
					cout <<"." << flush;

				if(!ready){ //if it was a nak, resend packet
					if(verbose){
						cout << "NAK... resending\n";
						cout << "bytesLeft: " << bytesLeft << endl;
					}	
					if(sendto(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *) &sendAddr, sizeof(sendAddr)) < 0){
						perror("error in sendto");
						exit(-1);
					}
				}

			}while(!ready);

			//cout << "ACK\n";
			bytesLeft -= bytesToSend;
			//cout << bytesLeft << " BytesLeft\n";
			sequenceNumber++;
		}

		double seconds = difftime(time(NULL), timer);

		cout << seconds << " seconds. (" << seconds/fileSize << " Bytes per second)\n";
		transactionNumber++;

	}while(persistent);    
    return EXIT_SUCCESS;
}