#NAME: Darren Lu                                           
#EMAIL: darrenlu3@ucla.edu
#ID: 205394473

lab1a: lab1a.c
	gcc -g -Wall -Wextra lab1a.c -o lab1a

clean:	
	rm -f lab1a lab1a-205394473.tar.gz

dist: lab1a.c README Makefile
	tar -cvzf lab1a-205394473.tar.gz README Makefile lab1a.c
