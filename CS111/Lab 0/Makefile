#NAME: Darren Lu                                           
#EMAIL: darrenlu3@ucla.edu
#ID: 205394473

lab0: lab0.c
	gcc -g -Wall -Wextra lab0.c -o lab0

check: smoketest.sh
	./smoketest.sh

clean:
	rm -f lab0 a.txt b.txt c.txt lab0-205394473.tar.gz 

dist: lab0.c smoketest.sh README backtrace.png breakpoint.png Makefile
	tar -cvzf lab0-205394473.tar.gz lab0.c smoketest.sh README backtrace.png breakpoint.png Makefile
