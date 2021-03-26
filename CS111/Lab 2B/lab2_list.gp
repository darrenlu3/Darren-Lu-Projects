#! /usr/bin/gnuplot
#
# purpose:
#	 generate data reduction graphs for the multi-threaded list project
#
# input: lab2b_list.csv
#	1. test name
#	2. # threads
#	3. # iterations per thread
#	4. # lists
#	5. # operations performed (threads x iterations x (ins + lookup + delete))
#	6. run time (ns)
#	7. run time per operation (ns)
#
# output:
#	lab2_list-1.png ... cost per operation vs threads and iterations
#	lab2_list-2.png ... threads and iterations that run (un-protected) w/o failure
#	lab2_list-3.png ... threads and iterations that run (protected) w/o failure
#	lab2_list-4.png ... cost per operation vs number of threads
#
# Note:
#	Managing data is simplified by keeping all of the results in a single
#	file.  But this means that the individual graphing commands have to
#	grep to select only the data they want.
#
#	Early in your implementation, you will not have data for all of the
#	tests, and the later sections may generate errors for missing data.
#

# general plot parameters
set terminal png
set datafile separator ","

# how many threads/iterations we can run without failure (w/o yielding)
set title "List-1: Total number of operations per second"
set xlabel "Threads"
set logscale x 2
set ylabel "Total number of operations per second"
set logscale y 10
set output 'lab2b_1.png'

# grep out only multi threaded, mutex and spin-protected, non-yield results
plot \
     "< grep -E 'list-none-m,[0-9]+,1000,1,' lab2b_list.csv" using ($2):(1000000000/($6)) \
	title 'mutex' with linespoints lc rgb 'red', \
     "< grep -E 'list-none-s,[0-9]+,1000,1,' lab2b_list.csv" using ($2):(1000000000/($6)) \
	title 'spin-lock' with linespoints lc rgb 'green'

set title "List-2: Wait for lock time and average time per operation against number of competing threads"
set xlabel "Threads"
set logscale x 2
set ylabel "Total number of operations per second/Time spent waiting for lock"
set logscale y 10
set output 'lab2b_2.png'

# grep out multi threaded, mutex protected non yield results
plot \
     "< grep -E 'list-none-m,[0-9]+,1000,1,' lab2b_list.csv" using ($2):($8) \
        title 'Wait for lock time' with linespoints lc rgb 'red', \
     "< grep -E 'list-none-m,[0-9]+,1000,1,' lab2b_list.csv" using ($2):($6) \
        title 'Average time per operation' with linespoints lc rgb 'red'

set title "List-3: Protected Iterations that run without failure"
unset logscale x
set xrange [0:4]
set xlabel "Yields"
set xtics("" 0, "no synchronization" 1, "spin lock" 2, "mutex lock" 3, "" 4)
set ylabel "successful iterations"
set logscale y 10
set output 'lab2b_3.png'

plot \
     "< grep 'list-id-none' lab2b_list.csv" using (1):($3) \
        with points lc rgb "red" title "unprotected, T=4", \
     "< grep 'list-id-s' lab2b_list.csv" using (2):($3) \
        with points lc rgb "red" title "spin lock, T=4", \
     "< grep 'list-id-m' lab2b_list.csv" using (3):($3) \
        with points lc rgb "red" title "mutex lock, T=4"

unset xtics
set xtics

set title "List-4: Aggregated throughput vs Number of threads"
set xlabel "Threads"
set xrange [0:20]
set ylabel "Total number of operations per second"
set logscale y 10
set output 'lab2b_4.png'

# grep out only multi threaded, mutex-protected, non-yield results
plot \
     "< grep -E 'list-none-m,[0-9]+,[0-9]+,1,' lab2b_list.csv" using ($2):(1000000000/($7))\
        title 'mutex, list=1' with linespoints lc rgb 'red', \
     "< grep -E 'list-none-m,[0-9]+,[0-9]+,4,' lab2b_list.csv" using ($2):(1000000000/($7))\
        title 'mutex, list=4' with linespoints lc rgb 'red', \
     "< grep -E 'list-none-m,[0-9]+,[0-9]+,8,' lab2b_list.csv" using ($2):(1000000000/($7))\
        title 'mutex, list=8' with linespoints lc rgb 'red', \
     "< grep -E 'list-none-m,[0-9]+,[0-9]+,16,' lab2b_list.csv" using ($2):(1000000000/($7))        title 'mutex, list=16' with linespoints lc rgb 'red'

set title "List-5: Aggregated throughput vs Number of threads"
set xlabel "Threads"
set xrange [0:20]
set ylabel "Total number of operations per second"
set logscale y 10
set output 'lab2b_5.png'

# grep out only multi threaded, spin-protected, non-yield results
plot \
     "< grep -E 'list-none-s,[0-9]+,[0-9]+,1,' lab2b_list.csv" using ($2):(1000000000/($7))\
        title 'mutex, list=1' with linespoints lc rgb 'red', \
     "< grep -E 'list-none-s,[0-9]+,[0-9]+,4,' lab2b_list.csv" using ($2):(1000000000/($7))\
        title 'mutex, list=4' with linespoints lc rgb 'red', \
     "< grep -E 'list-none-s,[0-9]+,[0-9]+,8,' lab2b_list.csv" using ($2):(1000000000/($7))\
        title 'mutex, list=8' with linespoints lc rgb 'red', \
     "< grep -E 'list-none-s,[0-9]+,[0-9]+,16,' lab2b_list.csv" using ($2):(1000000000/($7))        title 'mutex, list=16' with linespoints lc rgb 'red'