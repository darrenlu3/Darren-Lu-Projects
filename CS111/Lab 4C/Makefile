#NAME: Darren Lu                                           
#EMAIL: darrenlu3@ucla.edu
#ID: 205394473

build: lab4c_tcp lab4c_tls


lab4c_tcp: lab4c_tcp.c
	gcc -g -Wall -Wextra -lmraa -lm -o lab4c_tcp lab4c_tcp.c

lab4c_tls: lab4c_tls.c
	gcc -g -Wall -Wextra -lmraa -lm -o lab4c_tls lab4c_tls.c -lssl -lcrypto

clean:
	rm -f lab4c-205394473.tar.gz lab4c_tcp lab4c_tls

dist: README Makefile lab4c_tcp.c lab4c_tls.c
	tar -cvzf lab4c-205394473.tar.gz README Makefile lab4c_tcp.c lab4c_tls.c
