#!/usr/local/bin/bash
#NAME: Darren Lu                                          
#EMAIL: darrenlu3@ucla.edu                                  
#ID: 205394473 

#test script for project 2b
> lab2b_list.csv

for i in 1 2 4 8 12 16 24
do
    ./lab2_list --iterations=1000 --threads=$i --sync=m  >> lab2b_list.csv
    ./lab2_list --iterations=1000 --threads=$i --sync=s  >> lab2b_list.csv
done

for i in 1 4 8 #12 16
do
    for j in 1 2 4 #8 16
    do
	./lab2_list --iterations=$j --threads=$i --lists=4 --yield=id  >> lab2b_list.csv
	#./lab2_list --iterations=$j --threads=$i --lists=4 --yield=id --sync=m  >> lab2b_list.csv
    done
done

for i in 1 4 8 12 16
do
    for j in 10 16 20 40 80
    do
        ./lab2_list --iterations=$j --threads=$i --lists=4 --sync=m --yield=id >> lab2b_list.csv
        ./lab2_list --iterations=$j --threads=$i --lists=4 --sync=s --yield=id  >> lab2b_list.csv
    done
done

for i in 1 2 4 8 12 16
do
    for j in 1 4 8 16
    do
        ./lab2_list --iterations=1000 --threads=$i --lists=$j --sync=m >> lab2b_list.csv
        ./lab2_list --iterations=1000 --threads=$i --lists=$j --sync=s >> lab2b_list.csv
    done
done
