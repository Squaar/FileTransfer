all: server client 

server: server.cpp CS450Header.h share.h
	g++ -Wall server.cpp -o server
#	cp server ..

client: client.cpp CS450Header.h share.h
	g++ -Wall client.cpp -o client

gdb: server.cpp client.cpp CS450Header.h share.h
	g++ -Wall -g server.cpp -o server
	g++ -Wall -g client.cpp -o client
#	cp server ..

clean:
	rm server
	rm client
