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
#include <list>
#include <errno.h>
#include <fstream>
#include <signal.h>

#include "CS450Header.h"
#include "share.h"

using namespace std;

void alarmHandler(int sig);
void sendPackets();
void printWindow();

std::list<Packet> window;
int sockfd;
struct sockaddr_in sendAddr;
int verbose;

const static int TIMEOUT_SEC = 3;
const static int WINDOW_SIZE = 5;

//CS450VA - 54.84.21.227
//CS450OR - 54.213.83.180
//CS450CA - 54.193.35.191
//CS450EU - 54.194.234.13
//CS450TO - 54.199.136.22
//server port - 54323
//relay port - 54322

int main(int argc, char *argv[])
{
    // User Input
    cout << "Matt Dumford - mdumfo2@uic.edu\n\n";

	string server;
	if(argc > 1)
		server = argv[1];	
	else
		server = "127.0.0.1";

	string port;
	if(argc > 2)
		port = argv[2];
	else
		port = "54323";

	string relay;
	if(argc > 3)
		relay = argv[3];
	else
		relay = "none";

	string relayPort;
	if(argc > 4)
		relayPort = argv[4];
	else
		relayPort = "54322";

	string myPort;
	if(argc > 5)
		myPort = argv[5];
	else
		myPort = "54324";

	string garbleChance;
	if(argc > 6)
		garbleChance = argv[6];
	else
		garbleChance = "0";

	string dropChance = garbleChance;
	string dupeChance = garbleChance;
	string delayChance = garbleChance;

	// garbleChance = "0";
	// dropChance = "20";
	// dupeChance = "0";
	// delayChance = "0";
	
	int persistent = 0;
	int saveFile = 0;
	verbose = 0;
	if(argc > 7){
		for(int i=7; i<argc; i++){
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


    int transactionNumber = (int) time(NULL);
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
		memset(&sendAddr, 0, sizeof(sendAddr));
		 
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

		time_t timer = time(NULL); //timing the overall file.

		//====================================================================================
		//======================= EVERYTHING ABOVE SHOULD BE FINE! ===========================
		//============================== (It's all setup) ====================================
		//====================================================================================

		//	STEPS:
		/*	1. set timer for oldest packet sent.
			2. send N packets.
			3. wait for N responses.
			4. if timer expires or recieve bad ack, send packets again.
			5. move window up 1 for every packet successfully acked. 
			6. set new timer for every oldest packet.
		*/

		//set up alarm
		signal(SIGALRM, alarmHandler);
		
	    int sequenceNumber = 1;
		int windowPos = 1;
		int bytesLeft = fileSize; //how many bytes still need to be acked
		int bytesToPackage = fileSize; 	//how many bytes have been put into packets
										//(might not be acked yet)

		while(bytesLeft > 0){

			Packet packet;

			//create packets
			while(bytesToPackage > 0 && window.size() < WINDOW_SIZE){
				int bytesToSend = (bytesToPackage < BLOCKSIZE) ? bytesToPackage : BLOCKSIZE;

				memset(&packet, 0, sizeof(packet));

				//fill in packet fields
				packet.header.version = 7;
				packet.header.UIN = 675005893;
				packet.header.transactionNumber = transactionNumber;
				packet.header.sequenceNumber = sequenceNumber; //needs to start at one for some reason
				packet.header.from_IP = myIP;
				packet.header.to_IP = toIP;
				packet.header.trueFromIP = myIP;
				packet.header.trueToIP = toIP;
				packet.header.nbytes = bytesToSend;
				packet.header.nTotalBytes = fileSize;
				strcpy(packet.header.filename, filePath.c_str());
				packet.header.from_Port = atoi(myPort.c_str());
				packet.header.to_Port = atoi(port.c_str());
				//put checksum in after everything else
				packet.header.HW_number = 3;
				packet.header.packetType = 1;
				packet.header.saveFile = saveFile;
				packet.header.dropChance = atoi(dropChance.c_str());
				packet.header.dupeChance = atoi(dupeChance.c_str());
				packet.header.garbleChance = atoi(garbleChance.c_str());
				packet.header.delayChance = atoi(delayChance.c_str());
				packet.header.windowSize = WINDOW_SIZE;
				packet.header.protocol = 31;

				const char *ACCC = "mdumfo2";
	       		memcpy(&packet.header.ACCC, ACCC, strlen(ACCC));

	       		//copy data into packet
				memcpy(&packet.data, file + ((sequenceNumber-1)*BLOCKSIZE), bytesToSend);

				//create checksum and put in header
				packet.header.checksum = calcChecksum((void *) &packet, sizeof(Packet));

				//make sure checksum worked
				uint16_t newcheck = calcChecksum((void *) &packet, sizeof(packet));
				if(newcheck != 0){
					cout << "Bad checksum... Exiting.\n";
					exit(-1);
				}

				networkizeHeader(&packet.header);
				window.push_back(packet);
				sequenceNumber++;
				bytesToPackage -= bytesToSend;
			}

			//send packets
			std::list<Packet>::iterator it;
			sendPackets();

			//recieve responses
			it = window.begin();
			int currentWindowSize = window.size();
			for(int i=0; i<currentWindowSize; i++){

				//get response from server
				struct sockaddr_in responseAddr;
				memset(&responseAddr, 0, sizeof(responseAddr));
				Packet response;
				socklen_t responseAddrLen = sizeof(responseAddr);

				int readBytes = recvfrom(sockfd, &response, sizeof(response), 0, (struct sockaddr *) &responseAddr, &responseAddrLen);

				if(readBytes < 0){
					perror("Error in recvfrom");
					cout << "Bytes recieved: " << readBytes << endl;
					exit(-1);
				}

				deNetworkizeHeader(&response.header);

				if(response.header.ackNumber >= windowPos && response.header.ackNumber < windowPos + WINDOW_SIZE){
					if(verbose)
						cout << "good ack: "  << response.header.ackNumber << endl;

					//for(int j=0; j<response.haeder.ackNumber - windowPos + 1; j++){
					while(windowPos <= response.header.ackNumber){
						bytesLeft -= BLOCKSIZE;
						it = window.erase(it);
						windowPos++;
						i++;
					}	
				}
				else{
					if(verbose){
						if(response.header.ackNumber != windowPos){
							cout << "Bad ack: " << response.header.ackNumber << " expected: " 
								<< windowPos << endl;
							//printWindow();
						}
					}
				}
			}
		}

		//===================================================================================
		//========================= EVERYTHING BELOW IS FINE TOO ============================
		//===================================================================================

		double seconds = difftime(time(NULL), timer);

		cout << seconds << " seconds. (" << seconds/fileSize << " Bytes per second)\n";
		transactionNumber++;

	}while(persistent);    
    return EXIT_SUCCESS;
}

void alarmHandler(int sig){
	//resend packets
	sendPackets();
	signal(SIGALRM, alarmHandler);
}

void sendPackets(){
	std::list<Packet>::iterator it;
	for(it=window.begin(); it!=window.end(); it++){
		if(sendto(sockfd, &(*it), sizeof(Packet), 0, (struct sockaddr *) &sendAddr, sizeof(sendAddr)) < 0){
			perror("error in sendto");
			exit(-1);
		}

		if(verbose)
			cout << "Sent: " << ntohl((*it).header.sequenceNumber) << endl;

		if(it == window.begin())
			alarm(TIMEOUT_SEC);
	}
}

void printWindow(){
	std::list<Packet>::iterator it;
	cout << "\tCurrent Window: ";
	for(it=window.begin(); it!=window.end(); it++){
		cout << ntohl((*it).header.sequenceNumber) << ", ";
	}
	cout << endl;
}