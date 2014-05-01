Matt Dumford - mdumfo2@uic.edu

Client
======
client [server] [port] [relay] [relayPort] [clientPort] [garbleChance] [options]

- server - Where you want to send files to. Defaults to localhost.
- port - The port the server is listening to. Defaults to 54323.
- relay - Address of relay to send files through. Enter "none" to use no relay. Defaults to none.
- relayPort - The port the relay is running on. Defaults to 54322
- clientPort - The port that this client is to run on. Defaults to 54324
- garbleChance - the chance (%) that the relay will garble/drop/dupe/delay the data in the packet. This will only work if a relay is being used! Defaults to 0.

Options -

- -s, -S - Instructs the server to save the file.	
- -p, -P - Persistent. The server will keep the connection with the client open so you can send more files.
- -v, -V - Instructs the client to be more verbose with its output. 

These arguments must be given in this order. In order to use later arguments, values must be given for the previous ones. The options, however, can be given in any order or not at all, but they must be given last.

The user will be asked for a file to send once the client is started up.

Server
======
server [port] [options]

- port - The port to listen on. Defaults to 54321.

Options -

- -v, -V - Instructs the server to be more verbose with its output.

If the server is instructed to save a file it recieves, it will save them into the current directory with the name given to the client. It will overwrite files with the same name!

Known Problems
==============

- The client/server will not work with a garble chance higher than 0%
