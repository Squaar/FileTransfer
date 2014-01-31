/*  server.cpp

    acknowledgement after the entire file has been received.
    
    Two possible options for closing the connection:
        (1) Send a close command along with the acknowledgement and then close.
        (2) Hold the connection open waiting for additional files / commands.
        The "persistent" field in the header indicates the client's preference.
        
    Written by _________________ January 2014 for CS 361
*/


#include <cstdlib>
#include <iostream>

#include "CS450Header.h"

using namespace std;

int main(int argc, char *argv[])
{
    
    // User Input
    
    /* Check for the following from command-line args, or ask the user:
        
        Port number to listen to.  Default = 54321.
    */
    
    //  Call SOCKET to create a listening socket
    //  Call BIND to bind the socket to a particular port number
    //  Call LISTEN to set the socket to listening for new connection requests.
    
    // For HW1 only handle one connection at a time
    
    // Call ACCEPT to accept a new incoming connection request.
    // The result will be a new socket, which will be used for all further
    // communications with this client.
    
    // Call RECV to read in one CS450Header struct
    
    // Then call RECV again to read in the bytes of the incoming file.
    //      If "saveFile" is non-zero, save the file to disk under the name
    //      "filename".  Otherwise just read in the bytes and discard them.
    //      If the total # of bytes exceeds a limit, multiple RECVs are needed.
    
    // Send back an acknowledgement to the client, indicating the number of 
    // bytes received.  Other information may also be included.
    
    // If "persistent" is zero, then include a close command in the header
    // for the acknowledgement and close the socket.  Go back to ACCEPT to 
    // handle additional requests.  Otherwise keep the connection open and
    // read in another Header for another incoming file from this client.
    
    
    system("PAUSE");
    return EXIT_SUCCESS;
}
