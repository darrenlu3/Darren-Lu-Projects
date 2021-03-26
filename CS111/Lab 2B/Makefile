#NAME: Darren Lu                                           
#EMAIL: darrenlu3@ucla.edu
#ID: 205394473

build: lab2_list


lab2_list: lab2_list.c SortedList.c
	gcc -g -pthread -Wall -Wextra lab2_list.c SortedList.c -o lab2_list

tests: build list_test.sh
#	./add_test.sh
	./list_test.sh

profile: build
	LD_PRELOAD=/usr/lib64/libprofiler.so CPUPROFILE=./raw.perf ./lab2_list --threads=12 --iterations=1000 --sync=s
	pprof --list=add_thread lab2_list raw.perf > profile.out

graphs: tests lab2_list.gp
#gnuplot lab2_add.gp
	gnuplot lab2_list.gp

clean:
	rm -f lab2_add lab2_list lab2b-205394473.tar.gz raw.perf

dist: graphs profile README lab2_list.gp Makefile lab2_list.c SortedList.c SortedList.h list_test.sh
	tar -cvzf lab2b-205394473.tar.gz README lab2_list.gp Makefile lab2b_list.csv lab2_list.c SortedList.c SortedList.h list_test.sh profile.out lab2b_1.png lab2b_2.png lab2b_3.png lab2b_4.png lab2b_5.png 
