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

#include "CS450Header.h"

using namespace std;

int main(int argc, char *argv[])
{
    // User Input
    
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
    
    // Use FSTAT and MMAP to map the file to a memory buffer.  That will let the
    // virtual memory system page it in as needed instead of reading it byte
    // by byte.

	cout << "What file would you like to send?\n";
	string filePath;
	cin >> filePath;

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

	cout <<"poop\n";

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

	if(getaddrinfo(server.c_str(), port.c_str(), &hints, &res) !=0){
		perror("Error getting addrinfo");
		exit(-1);
	}
	
	int sockfd = socket(res->ai_family, res->ai_socktype, res-> ai_protocol);
	if(sockfd == -1){
		perror("Error creating socket");
		exit(-1);
	}

	if(connect(sockfd, res->ai_addr, res->ai_addrlen) != 0){
		//server might just not be ready?
		perror("Error connecting");
		exit(-1);
	}


    // Note the time before beginning transmission

	time_t start = time(NULL);
    
    // Create a CS450Header object, fill in the fields, and use SEND to write
    // it to the socket.
    
	CS450Header header;
	memset(&header, 0, sizeof(header));

	header.HW_number = 1;
	header.transactionNumber = 1;

	char *ACCC = "mdumfo2";
	memcpy(header.ACCC, ACCC, strlen(ACCC));

	//header.filename =  //NEED TO GET NAME
	//header.From_IP = 
	//header.To_IP = 
	header.packetType = 1;
	header.nbytes = fileSize;
	
	size_t bytesSent = send(sockfd, &header, sizeof(header), 0);
	if(bytesSent != sizeof(header)){
		perror("Something went wrong sending file");
		exit(-1);
	}	

    // Use SEND to send the data file.  If it is too big to send in one gulp
    // Then multiple SEND commands may be necessary.
   
	bytesSent = send(sockfd, file, fileSize, 0);
	if(bytesSent != sizeof(header)){
		perror("Something went wrong sending file");
		//exit(-1);
	}
 
    // Use RECV to read in a CS450Header from the server, which should contain
    // some acknowledgement information.  
    
    // Calculate the round-trip time and
    // bandwidth, and report them to the screen along with the size of the file
    // and output echo of all user input data.
    
	time_t end = time(NULL);

	cout << end-start << " seconds.\n";

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
    
    return EXIT_SUCCESS;
}
