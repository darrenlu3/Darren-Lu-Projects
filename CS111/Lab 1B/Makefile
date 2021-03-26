#NAME: Darren Lu                                           
#EMAIL: darrenlu3@ucla.edu
#ID: 205394473

all: lab1b-client lab1b-server

lab1b-client: lab1b-client.c
	gcc -g -Wall -Wextra -lz lab1b-client.c -o lab1b-client

lab1b-server: lab1b-server.c
	gcc -g -Wall -Wextra -lz lab1b-server.c -o lab1b-server

clean:	
	rm -f lab1b-client lab1b-server lab1b-205394473.tar.gz

dist: lab1b-client.c lab1b-server.c README Makefile
	tar -cvzf lab1b-205394473.tar.gz README Makefile lab1b-client.c lab1b-server.c
