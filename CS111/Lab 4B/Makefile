#NAME: Darren Lu                                           
#EMAIL: darrenlu3@ucla.edu
#ID: 205394473

build: lab4b


lab4b: lab4b.c
	gcc -g -Wall -Wextra -lmraa -lm -o lab4b lab4b.c

dummy: lab4b.c
	gcc -g -Wall -Wextra -DDUMMY -lm -o lab4b lab4b.c

check:

clean:
	rm -f lab4b-205394473.tar.gz lab4b

dist: README Makefile lab4b.c
	tar -cvzf lab4b-205394473.tar.gz README Makefile lab4b.c
