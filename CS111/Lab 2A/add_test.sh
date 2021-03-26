#!/usr/local/bin/bash
#NAME: Darren Lu                                          
#EMAIL: darrenlu3@ucla.edu                                  
#ID: 205394473 

#test script for part 1 of project 2a
 > lab2_add.csv
#for i in 1 2 3 4 5
#do
./lab2_add --iterations=100 --threads=10 >> lab2_add.csv
./lab2_add --iterations=1000 --threads=10 >> lab2_add.csv
./lab2_add --iterations=2500 --threads=10 >> lab2_add.csv
./lab2_add --iterations=10000 --threads=10 >> lab2_add.csv
./lab2_add --iterations=100000 --threads=10 >> lab2_add.csv
#done

for i in 2 4 8 12
do
./lab2_add --threads=$i --iterations=10 >> lab2_add.csv
./lab2_add --threads=$i --iterations=20 >> lab2_add.csv
./lab2_add --threads=$i --iterations=40 >> lab2_add.csv
./lab2_add --threads=$i --iterations=80 >> lab2_add.csv
./lab2_add --threads=$i --iterations=100 >> lab2_add.csv
./lab2_add --threads=$i --iterations=1000 >> lab2_add.csv
./lab2_add --threads=$i --iterations=10000 >> lab2_add.csv
./lab2_add --threads=$i --iterations=100000 >> lab2_add.csv
done

for i in 2 4 8 12
do
./lab2_add --threads=$i --iterations=10 --yield >> lab2_add.csv
./lab2_add --threads=$i --iterations=20 --yield >> lab2_add.csv
./lab2_add --threads=$i --iterations=40 --yield >> lab2_add.csv
./lab2_add --threads=$i --iterations=80 --yield >> lab2_add.csv
./lab2_add --threads=$i --iterations=100 --yield >> lab2_add.csv
./lab2_add --threads=$i --iterations=1000 --yield >> lab2_add.csv
./lab2_add --threads=$i --iterations=10000 --yield >> lab2_add.csv
./lab2_add --threads=$i --iterations=100000 --yield >> lab2_add.csv
done

for i in 2 4 6 8
do
./lab2_add --iterations=100 --threads=$i >> lab2_add.csv
./lab2_add --iterations=1000 --threads=$i >> lab2_add.csv
./lab2_add --iterations=10000 --threads=$i >> lab2_add.csv
./lab2_add --iterations=100000 --threads=$i >> lab2_add.csv
./lab2_add --iterations=100 --threads=$i --yield >> lab2_add.csv
./lab2_add --iterations=1000 --threads=$i --yield >> lab2_add.csv
./lab2_add --iterations=10000 --threads=$i --yield >> lab2_add.csv
./lab2_add --iterations=100000 --threads=$i --yield >> lab2_add.csv
done

for i in {10..100000..100}
do
./lab2_add --iterations=$i >>lab2_add.csv
done

for i in 2 4 8 12
do
./lab2_add --threads=$i --iterations=10000 --yield --sync=m  >> lab2_add.csv
./lab2_add --threads=$i--iterations=1000 --yield --sync=s  >> lab2_add.csv
./lab2_add --threads=$i--iterations=10000 --yield --sync=c  >> lab2_add.csv
done

for i in 2 4 8 12
do
    for j in 10 20 40 80 100 500 1000
    do
	./lab2_add --threads=$i --iterations=$j --yield >> lab2_add.csv
	./lab2_add --threads=$i --iterations=$j --yield --sync=m  >> lab2_add.csv
	./lab2_add --threads=$i --iterations=$j --yield --sync=s  >> lab2_add.csv
	./lab2_add --threads=$i --iterations=$j --yield --sync=c  >> lab2_add.csv
    done
done

for i in 1 2 4 8 12
do
    for j in 10000
    do
        ./lab2_add --threads=$i --iterations=$j  >> lab2_add.csv
        ./lab2_add --threads=$i --iterations=$j --sync=m  >> lab2_add.csv
        ./lab2_add --threads=$i --iterations=$j --sync=s  >> lab2_add.csv
        ./lab2_add --threads=$i --iterations=$j --sync=c  >> lab2_add.csv
    done
done
