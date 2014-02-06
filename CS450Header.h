/*  CS450Header.h

    this file defines the format of the header block that is to precede each
    block of data transmitted for the CS 450 HW in Spring 2014.
    
*/


typedef struct{
	//COMPILER YELLS ABOUT VARIABLE NAMED VERSION
    //int version = 2;  // Initial version.  Later versions may have more fields
    long int UIN; // Packets with unrecognized UINs will be dropped
    int HW_number; // e.g. 1 for HW1, etc. 
    int transactionNumber;  // To identify multiple parts of one transaction.
    char ACCC[ 10 ],  // your ACCC user ID name, e.g. astudent42
        filename[ 20 ];  // the name of the file being transmitted.
                         // i.e. the name where the server should store it.
    unsigned int From_IP, To_IP;  // Ultimate destination, not the relay
	int packetType; // 1 = file transfer; 2 = acknowledgement
    unsigned long nbytes; // Number of bytes of data to follow the header
    // float garbleChance, dropChance, dupeChance; // For later HW
    int relayCommand; // 1 = close connection
    int persistent;  // non-zero:  Hold connection open after this file 
    int saveFile;  // non-zero:  Save incoming file to disk.  Else discard it.
    
    // Students may change the following, so long as the total overall size
    // of the struct does not change.
    // I.e. you can add additional fields, but if you do, reduce the size of the
    // reserved array by an equal number of bytes.

	unsigned long bytesRecieved; //to report back to the client how many bytes were recieved

    char reserved[ 1000-sizeof(bytesRecieved) ];
    
    
} CS450Header;
