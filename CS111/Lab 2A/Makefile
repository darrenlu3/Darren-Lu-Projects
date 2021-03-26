#NAME: Darren Lu                                           
#EMAIL: darrenlu3@ucla.edu
#ID: 205394473

build: lab2_add lab2_list

lab2_add: lab2_add.c
	gcc -g -pthread -Wall -Wextra lab2_add.c -o lab2_add

lab2_list: lab2_list.c SortedList.c
	gcc -g -pthread -Wall -Wextra lab2_list.c SortedList.c -o lab2_list

tests: build add_test.sh list_test.sh
	./add_test.sh
	./list_test.sh

graphs: tests lab2_add.gp lab2_list.gp
	gnuplot lab2_add.gp
	gnuplot lab2_list.gp

clean:
	rm -f lab2_add lab2_list lab2a-205394473.tar.gz

dist: graphs lab2_add.c README lab2_add.gp lab2_list.gp Makefile lab2_list.c SortedList.c SortedList.h list_test.sh add_test.sh
	tar -cvzf lab2a-205394473.tar.gz lab2_add.c README lab2_add.gp lab2_list.gp Makefile lab2_add.csv lab2_list.csv lab2_list.c SortedList.c SortedList.h lab2_add-1.png lab2_add-2.png lab2_add-3.png lab2_add-4.png lab2_add-5.png lab2_list-1.png lab2_list-2.png lab2_list-3.png lab2_list-4.png list_test.sh add_test.sh
