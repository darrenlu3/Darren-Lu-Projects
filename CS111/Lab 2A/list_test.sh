#!/usr/local/bin/bash
#NAME: Darren Lu                                          
#EMAIL: darrenlu3@ucla.edu                                  
#ID: 205394473 

#test script for part 2 of project 2a
> lab2_list.csv

for i in 10 100 1000 10000 20000
do
./lab2_list --iterations=$i >> lab2_list.csv
done

for i in 2 4 8 12
do
    for j in 1 10 100 1000
    do
	./lab2_list --iterations=$j --threads=$i >> lab2_list.csv
    done
done

for i in 2 4 8 12
do
    for j in 1 10 100 1000
    do
	./lab2_list --iterations=$j --threads=$i --yield=i >> lab2_list.csv
	./lab2_list --iterations=$j --threads=$i --yield=d >> lab2_list.csv
	./lab2_list --iterations=$j --threads=$i --yield=il >> lab2_list.csv
	./lab2_list --iterations=$j --threads=$i --yield=dl >> lab2_list.csv
    done
done

for i in 2 4 8 12 32
do
    for j in 1 10 100 1000
    do
	./lab2_list --iterations=$j --threads=$i --yield=i --sync=m >> lab2_list.csv
        ./lab2_list --iterations=$j --threads=$i --yield=d --sync=m >> lab2_list.csv
        ./lab2_list --iterations=$j --threads=$i --yield=il --sync=m >> lab2_list.csv
        ./lab2_list --iterations=$j --threads=$i --yield=dl --sync=m >> lab2_list.csv
	./lab2_list --iterations=$j --threads=$i --yield=i --sync=s >> lab2_list.csv
        ./lab2_list --iterations=$j --threads=$i --yield=d --sync=s >> lab2_list.csv
        ./lab2_list --iterations=$j --threads=$i --yield=il --sync=s >> lab2_list.csv
        ./lab2_list --iterations=$j --threads=$i --yield=dl --sync=s >> lab2_list.csv
    done
done

for i in 1 2 4 8 12 16 24
do
    ./lab2_list --iterations=1000 --threads=$i --sync=m >>lab2_list.csv
    ./lab2_list --iterations=1000 --threads=$i --sync=s >>lab2_list.csv
done
