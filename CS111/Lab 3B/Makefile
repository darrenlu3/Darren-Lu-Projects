#NAME: Darren Lu                                           
#EMAIL: darrenlu3@ucla.edu
#ID: 205394473

build: lab3b


lab3b: lab3b.py
	ln -s lab3b.py lab3b
	chmod u+rwx ./lab3b

clean:
	rm -f lab3b lab3b-205394473.tar.gz

dist: README lab3b.py Makefile
	tar -cvzf lab3b-205394473.tar.gz README Makefile lab3b.py
