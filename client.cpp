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

using namespace std;

void sendCloseHeader(int sockfd, int myIP, int toIP, int toPort);

int main(int argc, char *argv[])
{
    // User Input

    cout << "Matt Dumford - mdumfo2@uic.edu\n\n";
    
    /* Check for the following from command-line args, or ask the user:
        
        Destination ( server ) name and port number
        Relay name and port number.  If none, communicate directly with server
        File to transfer.  Use OPEN(2) to verify that the file can be opened
        for reading, and ask again if necessary.
    */

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

	string relayPort;
	if(argc > 4)
		relayPort = argv[4];
	else
		relayPort = "none";

	int persistent = 0;
	int saveFile = 0;
	if(argc > 5){
		for(int i=5; i<argc; i++){
			if(!strcmp(argv[i], "-p") || !strcmp(argv[i], "-P"))
				persistent = 1;
			if(!strcmp(argv[i], "-s") || !strcmp(argv[i], "-S"))
				saveFile = 1;
		}
	}

	int useRelay = 0;
	if(relay.compare("none"))
		useRelay = 1;

	char myHostname[128];
	gethostname(myHostname, sizeof(myHostname));
	struct hostent *myHe = gethostbyname(myHostname);
	u_int32_t myIP = ((struct in_addr **) myHe->h_addr_list)[0]->s_addr;

	struct hostent *toHe = gethostbyname(server.c_str());
	u_int32_t toIP = ((struct in_addr **) toHe->h_addr_list)[0]->s_addr;


    int sockfd;
    int fresh = 1; //determines if this is the first loop on a persistent connection
    int transactionNumber = 1;

	do{
	    // Use FSTAT and MMAP to map the file to a memory buffer.  That will let the
	    // virtual memory system page it in as needed instead of reading it byte
	    // by byte.

		cout << "Enter a file to send, or exit to exit.\n>";
		string filePath;
		cin >> filePath;

		if(filePath.compare("exit") == 0){
			if(!fresh){
				sendCloseHeader(sockfd, myIP, toIP, atoi(port.c_str()));
				close(sockfd);
			}
			exit(0);
		}

		int fd = open(filePath.c_str(), O_RDONLY);
		if(fd == -1){
			perror("Error opening file");
			if(!fresh)
				sendCloseHeader(sockfd, myIP, toIP, atoi(port.c_str()));
			exit(-1);
		}

		struct stat stats;
		if(fstat(fd, &stats) < 0){
			perror("Error in fstat");
			if(!fresh)
				sendCloseHeader(sockfd, myIP, toIP, atoi(port.c_str()));
			exit(-1);
		}

		size_t fileSize = stats.st_size;

		cout << "Filesize: " << fileSize << endl;		

		char *file = (char *) mmap(NULL, fileSize, PROT_READ, MAP_SHARED, fd, 0);
		if(file == MAP_FAILED){
			perror("Error in mmap");
			if(!fresh)
				sendCloseHeader(sockfd, myIP, toIP, atoi(port.c_str()));
			exit(-1);
		}

	    if(fresh){
	    	// Open a Connection to the server ( or relay )  TCP for the first HW
    	    // call SOCKET and CONNECT and verify that the connection opened.
    	    
    		// protocols are in /etc/protocols
    		// 0-IP --beej uses this i think
    		// 6-TCP
    		struct addrinfo hints;
    		struct addrinfo *res;
    
    		memset(&hints, 0, sizeof(hints));
    		hints.ai_family = AF_INET;
    		hints.ai_socktype = SOCK_STREAM;

    		if(!useRelay){
	    		if(getaddrinfo(server.c_str(), port.c_str(), &hints, &res) !=0){
	    			perror("Error getting addrinfo");
	    			exit(-1);
	    		}
	    	}
	    	else{
    			if(getaddrinfo(relay.c_str(), relayPort.c_str(), &hints, &res) !=0){
	    			perror("Error getting addrinfo");
	    			exit(-1);
	    		}
	    	}

    		sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    		if(sockfd == -1){
    			perror("Error creating socket");
    			exit(-1);
    		}
    
    		if(connect(sockfd, res->ai_addr, res->ai_addrlen) != 0){
    			perror("Error connecting");
    			exit(-1);
    		}
    	}


	    // Note the time before beginning transmission

		clock_t start = clock();
	    
	    // Create a CS450Header object, fill in the fields, and use SEND to write
	    // it to the socket.
	    
		CS450Header header;
		memset(&header, 0, sizeof(header));

		header.UIN = 675005893;
		header.HW_number = 1;
		header.transactionNumber = transactionNumber;
		strcpy(header.filename, filePath.c_str());
		header.from_IP = myIP;
		header.to_IP = toIP;
		header.packetType = 1;
		header.nbytes = fileSize;
		header.persistent = persistent;
		header.saveFile = saveFile;
		header.from_Port = 54321;
		header.to_Port = atoi(port.c_str());
		
		networkizeHeader(&header);

		long bytesSent = send(sockfd, &header, sizeof(header), 0);
		if(bytesSent == -1){
			perror("Something went wrong sending header");
			sendCloseHeader(sockfd, myIP, toIP, atoi(port.c_str()));
			exit(-1);
		}	

	    // Use SEND to send the data file.  If it is too big to send in one gulp
	    // Then multiple SEND commands may be necessary.
	   
		bytesSent = send(sockfd, file, fileSize, 0);
		if(bytesSent == -1){
			perror("Something went wrong sending file");
			sendCloseHeader(sockfd, myIP, toIP, atoi(port.c_str()));
			exit(-1);
		}
	 
	    // Use RECV to read in a CS450Header from the server, which should contain
	    // some acknowledgement information.  

	    CS450Header response;
	    
	    long bytesRecieved = recv(sockfd, &response, sizeof(response), 0);
		if(bytesRecieved == -1){
			perror("Error recieving response header");
			sendCloseHeader(sockfd, myIP, toIP, atoi(port.c_str()));
			exit(-1);
		}

		deNetworkizeHeader(&response);
	    
	    // Calculate the round-trip time and
	    // bandwidth, and report them to the screen along with the size of the file
	    // and output echo of all user input data.
	    
		clock_t end = clock();
		double time = (end-start) / (double) CLOCKS_PER_SEC;

		cout << time << " seconds. (" << time/fileSize << " Bytes per second)\n";
		//cout << "Thats " << (end-start) / (sizeof(header)*2 + bytesSent) << " seconds per byte transmitted!\n";

	    // When the job is done, either the client or the server must transmit a
	    // CS450Header containing the command code to close the connection, and 
	    // having transmitted it, close their end of the socket.  If a relay is 
	    // involved, it will transmit the close command to the other side before
	    // closing its connections.  Whichever process receives the close command
	    // should close their end and also all open data files at that time.
	    
	    // If it is desired to transmit another file, possibly using a different
	    // destination, protocol, relay, and/or other parameters, get the
	    // necessary input from the user and try again.
	    
	    // When done, report overall statistics and exit.

		transactionNumber++;
	    fresh = 0;

	}while(persistent);

	sendCloseHeader(sockfd, myIP, toIP, atoi(port.c_str()));
	close(sockfd);
    
    return EXIT_SUCCESS;
}

void sendCloseHeader(int sockfd, int myIP, int toIP, int toPort){
	CS450Header header;
	memset(&header, 0, sizeof(header));

	header.UIN = 675005893;
	header.HW_number = 1;
	header.transactionNumber = 1;
	header.from_IP = myIP;
	header.to_IP = toIP;
	header.packetType = 1;
	header.relayCommand = 1;
	header.persistent = 0;
	header.to_Port = toPort;

	networkizeHeader(&header);
	
	long bytesSent = send(sockfd, &header, sizeof(header), 0);
	if(bytesSent == -1){
		perror("Something went wrong sending header");
		exit(-1);
	}
}