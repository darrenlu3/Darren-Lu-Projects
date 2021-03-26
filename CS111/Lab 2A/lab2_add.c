//NAME: Darren Lu                                           
//EMAIL: darrenlu3@ucla.edu
//ID: 205394473

#include <getopt.h>
#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sched.h>
#include <stdbool.h>

long long counter = 0;
int opt_yield;
void add(long long *pointer, long long value) {
  long long sum = *pointer + value;
  if (opt_yield) 
    sched_yield();
  *pointer = sum;
}

pthread_mutex_t mutexlock;
void add_mutex(long long *pointer, long long value) {
  long long sum = *pointer + value;
  if (opt_yield)
    sched_yield();
  *pointer = sum;
}

long lock = 0;
void add_spin(long long *pointer, long long value) {
  long long sum = *pointer + value;
  if (opt_yield)
    sched_yield();
  *pointer = sum;
}

void add_cmp_swap(long long *pointer, long long value) {
  long long prev,sum;
  // = *pointer + value;
  do{
    if (opt_yield)
      sched_yield();
    prev = *pointer;
    sum = prev + value;
  }while(__sync_bool_compare_and_swap(pointer, prev, sum) == false);
  long long a = 1, b = 2;
  __sync_val_compare_and_swap(&a,&b,2);
}

struct thread_args{
  int iterations;
  char sync;
};

void* add_thread(void *threadargs)
{
  struct thread_args *args;
  args = (struct thread_args*)threadargs;
  long iter = (long)args->iterations;
  char mode = (char)args->sync;
  //fprintf(stderr, "%ld\n", iter);
  int i;
  if (mode == 'n'){
    //fprintf(stderr,"none");
    for (i = 0; i< iter; i++) add(&counter, 1);
    for (i = 0; i< iter; i++) add(&counter, -1);
  }
  else if (mode == 'm'){
    //fprintf(stderr,"mutex");
    pthread_mutex_lock(&mutexlock);
    for (i = 0; i< iter; i++) add_mutex(&counter, 1);
    for (i = 0; i< iter; i++) add_mutex(&counter, -1);
    pthread_mutex_unlock(&mutexlock);
  }
  else if (mode == 's'){
    //fprintf(stderr,"spin");
    while(__sync_lock_test_and_set (&lock,1));
    for (i = 0; i< iter; i++) add_spin(&counter, 1);
    for (i = 0; i< iter; i++) add_spin(&counter, -1);
    __sync_lock_release(&lock);
  }
  else if (mode == 'c'){
    //fprintf(stderr,"cmpswp");
    for (i = 0; i< iter; i++) add_cmp_swap(&counter, 1);
    for (i = 0; i< iter; i++) add_cmp_swap(&counter, -1);
  }
  pthread_exit(NULL);
}

static inline unsigned long get_nanosec_from_timespec(struct timespec* spec)
//function from discussion 1b week 4
{
  unsigned long ret = spec->tv_sec;
  ret = ret * 1000000000 + spec->tv_nsec;
  return ret;
}

int main(int argc, char *argv[]){
  int thread_num=1;
  long iter_num=1;
  char* output_tag = "add";
  char* sync_tag = "-none";
  char sync = 'n';
  //Parse options
  while(1){
    int option_index = 0;
    static struct option options[] = {
      {"threads=", required_argument, 0, 0},
      {"iterations=", required_argument, 0, 1},
      {"yield", no_argument, 0, 2},
      {"sync=", required_argument, 0, 3}
    };
    
    int c = getopt_long(argc, argv, "", options, &option_index);
    
    if (c== -1) break;
    switch(c){
    case 0:
      if (optarg && (strlen(optarg)>0)){
	thread_num= atoi(optarg);
	//fprintf(stderr,"%d\n",thread_num);
      }
      else{
	fprintf(stderr, "Usage: %s [--threads=# (default 1)] [--iterations=# (default 1)] [--sync= (m for mutex, s for spin-lock, c for compare-and-swap)] [--yield]\n", argv[0]);
	_exit(1);
      }
      break;
    case 1:
      if (optarg && (strlen(optarg)>0)){
        iter_num= atoi(optarg);
        //fprintf(stderr,"%d\n",iter_num);
      }
      else{
	fprintf(stderr, "Usage: %s [--threads=# (default 1)] [--iterations=# (default 1)] [--sync= (m for mutex, s for spin-lock, c for compare-and-swap)] [--yield]\n", argv[0]);
        _exit(1);
      }
      break;
    case 2:
      opt_yield=1;
      output_tag="add-yield";
      //strcat("-yield", output_tag);
      break;
    case 3:
      if (optarg && strlen(optarg) == 1){
	if (optarg[0] == 'm'){ sync = 'm'; sync_tag = "-m";}
	else if (optarg[0] == 's'){ sync = 's'; sync_tag = "-s";}
	else if (optarg[0] == 'c'){ sync = 'c'; sync_tag = "-c";}
	else{
	  fprintf(stderr, "Unrecognized synchronization tag.\n");
	  _exit(1);
	}
      }
      else{
	  fprintf(stderr, "Unrecognized synchronization tag.\n");
	  _exit(1);
      }
      break;
    default:
      fprintf(stderr, "Usage: %s [--threads=# (default 1)] [--iterations=# (default 1)] [--sync= (m for mutex, s for spin-lock, c for compare-and-swap)] [--yield]\n", argv[0]);
      _exit(1);
    }    
  }

  //Get start time
  struct timespec start,stop;
  if(clock_gettime(CLOCK_MONOTONIC, &start) == -1){
    fprintf(stderr, "Error getting start time. Reason: %s", strerror(errno));
    _exit(1);
  }
  //printf("%ld\n", start.tv_nsec);
  //Start threading
  pthread_t thread[thread_num];
  struct thread_args targs;
  targs.iterations = iter_num;
  targs.sync = sync;
  int i;
  
  //initialize mutex
  pthread_mutex_init(&mutexlock, NULL);

  for(i = 0; i<thread_num; i++){
    if(pthread_create(&thread[i], NULL, add_thread, (void*) &targs ) != 0){
      fprintf(stderr, "Error creating thread number %d. Reason: %s", i+1, strerror(errno));
      _exit(1);
    }
  }
  for(i = 0; i<thread_num; i++){
    if(pthread_join(thread[i], NULL) != 0){
      fprintf(stderr, "Error joining thread number %d. Reason: %s", i+1, strerror(errno));
      _exit(1);
    }
  }

  //destroy mutex
  pthread_mutex_destroy(&mutexlock);

  //Get stop time
  if(clock_gettime(CLOCK_MONOTONIC, &stop) == -1){
    fprintf(stderr, "Error getting stop time. Reason: %s", strerror(errno));
    _exit(1);
  }
  int operations = thread_num*iter_num*2;
  //int time_taken = stop.tv_nsec-start.tv_nsec;
  long time_taken = get_nanosec_from_timespec(&stop) - get_nanosec_from_timespec(&start);
  char output_str[100];
  strcpy(output_str, output_tag);
  strcat(output_str, sync_tag);
  printf("%s,%d,%ld,%d,%ld,%ld,%lld\n",output_str,thread_num,iter_num,operations,time_taken,time_taken/operations,counter);

  /*
  printf("%c\n", sync);
  printf("%ld\n", start.tv_nsec);   
  printf("%d\n", thread_num);
  printf("%d\n", iter_num);
  printf("%d\n", opt_yield);
  printf("%ld\n", stop.tv_nsec);
  */
  
  return(0);
}
