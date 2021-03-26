#NAME: Darren Lu                                           
#EMAIL: darrenlu3@ucla.edu
#ID: 205394473

build: lab3a


lab3a: lab3a.c ext2_fs.h
	gcc -g -Wall -Wextra lab3a.c -o lab3a

clean:
	rm -f lab3a lab3a-205394473.tar.gz

dist: README lab3a.c Makefile ext2_fs.h
	tar -cvzf lab3a-205394473.tar.gz README Makefile lab3a.c ext2_fs.h
