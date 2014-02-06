/*  server.cpp
	
	This program receives a file from a remote client and sends back an
    acknowledgement after the entire file has been received.
    
    Two possible options for closing the connection:
        (1) Send a close command along with the acknowledgement and then close.
        (2) Hold the connection open waiting for additional files / commands.
        The "persistent" field in the header indicates the client's preference.
        
    Written by Matt Dumford January 2014 for CS 361
	mdumfo2@uic.edu
*/


#include <cstdlib>
#include <iostream>
#include <sys/types.h>
#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>

#include "CS450Header.h"

using namespace std;

int main(int argc, char *argv[])
{
    
    // User Input
    
    /* Check for the following from command-line args, or ask the user:
        Port number to listen to.  Default = 54321.
    */

	string port;
	if(argc > 1)
		port = argv[1];
	else
		port = "54321";
	 
    //  Call SOCKET to create a listening socket
    //  Call BIND to bind the socket to a particular port number
    //  Call LISTEN to set the socket to listening for new connection requests.
    
	struct addrinfo hints;
	struct addrinfo *res;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if(getaddrinfo(NULL, port.c_str(), &hints, &res) !=0){
		perror("Error getting addrinfo");
		exit(-1);
	}

	int listenSocket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if(listenSocket == -1){
		perror("Error creating listening socket");
		exit(-1);
	}

	if(bind(listenSocket, res->ai_addr, res->ai_addrlen) != 0){
		perror("Error binding");
		exit(-1);
	}

	if(listen(listenSocket, 20) != 0){
		perror("Error listening");
		exit(-1);
	}

	cout << "Listening on port " << port << ".\n";

    // For HW1 only handle one connection at a time
    
    // Call ACCEPT to accept a new incoming connection request.
    // The result will be a new socket, which will be used for all further
    // communications with this client.
    
	while(1){ //keep accepting new requests forever

		struct sockaddr_storage storage;
		socklen_t storage_size = sizeof(storage);
		int sockfd = accept(listenSocket, (struct sockaddr *) &storage, &storage_size);
		if(sockfd == -1){
			perror("Error accepting");
			exit(-1);
		}
	
	    // Call RECV to read in one CS450Header struct

		bool persist = true;
		while(persist){
		
			CS450Header header;
		
			long bytesRecieved = recv(sockfd, &header, sizeof(header), 0);
			if(bytesRecieved == -1){
				perror("Error recieving header");
				exit(-1);
			}
		
		    // Then call RECV again to read in the bytes of the incoming file.
		    //      If "saveFile" is non-zero, save the file to disk under the name
		    //      "filename".  Otherwise just read in the bytes and discard them.
		    //      If the total # of bytes exceeds a limit, multiple RECVs are needed.
		
			char file[header.nbytes];
		   
			bytesRecieved = recv(sockfd, file, header.nbytes, 0);
			if(bytesRecieved == -1){
				perror("Error recieving file");
				exit(-1);
			}

			printf("FILE RECIEVED:\n\n%s\n", file);
			if(header.saveFile){
				
			}
		 
		    // Send back an acknowledgement to the client, indicating the number of 
		    // bytes received.  Other information may also be included.
		
			CS450Header response;
			memset(&response, 0, sizeof(response));
		
			response.UIN = 675005893;
			response.HW_number = 1;

			const char *ACCC = "mdumfo2";
			memcpy(response.ACCC, ACCC, strlen(ACCC));

			response.packetType = 2;
			response.bytesRecieved = bytesRecieved;
		    
		    // If "persistent" is zero, then include a close command in the header
		    // for the acknowledgement and close the socket.  Go back to ACCEPT to 
		    // handle additional requests.  Otherwise keep the connection open and
		    // read in another Header for another incoming file from this client.
		    
			if(!header.persistent){
				response.relayCommand = 1;
				persist = false;
			}
			else
				persist = true;

			long bytesSent = send(sockfd, &response, sizeof(response), 0);
			if(bytesSent == -1){
				perror("Error sending response header");
				exit(-1);
			}
			
		}//end while(persist) for keeping connection open
	} //end while(1) for accepting   

    return EXIT_SUCCESS;
}
