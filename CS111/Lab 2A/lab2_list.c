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
#include "SortedList.h"
#include <signal.h>


long long counter = 0;
int opt_yield;
pthread_mutex_t mutexlock;
SortedList_t *listhead;
SortedListElement_t *pool;

void catchseg(){
  fprintf(stderr, "My segfault handler caught a segfault.\n");
  _exit(2);
}

struct thread_args{
  int threadNum;
  int iterations;
  char sync;
};
long lock = 0;
void* add_thread(void *threadargs)
{
  struct thread_args *args;
  args = (struct thread_args*)threadargs;
  int threadNum = (int)args->threadNum;
  long iter = (long)args->iterations;
  char mode = (char)args->sync;
  long startIndex = threadNum*iter;
  //fprintf(stderr, "%ld\n", iter);
  int i;
  //FUNCTIONALITY with help from discussion 1b week 4
  if (mode == 'm') pthread_mutex_lock(&mutexlock);
  if (mode == 's') while(__sync_lock_test_and_set(&lock,1));
  for(i = startIndex; i< startIndex + iter; i++) SortedList_insert(listhead, &pool[i]);
  int status = SortedList_length(listhead);
  if (status < 0){
    fprintf(stderr,"SortedList_length returned a negative value. Exiting.\n");
    _exit(2);
  }
  //fprintf(stderr, "Thread %d sorted list length %d\n",threadNum,status);
  SortedListElement_t* e;
  for(i = startIndex; i<startIndex + iter; i++){
    e = SortedList_lookup(listhead, (&pool[i])->key);
    if (e == NULL){
      fprintf(stderr,"SortedList_lookup could not find the provided element. Exiting.\n");
      _exit(2);
    }
    int g = SortedList_delete(e);
    if (g < 0){
      fprintf(stderr,"SortedList_delete returned a negative value. Exiting.\n");
      _exit(2);
    }
  }
  if (mode == 'm') pthread_mutex_unlock(&mutexlock);
  if (mode =='s') __sync_lock_release(&lock);
  pthread_exit(NULL);
}

static inline unsigned long get_nanosec_from_timespec(struct timespec* spec)
//function from discussion 1b week 4
{
  unsigned long ret = spec->tv_sec;
  ret = ret * 1000000000 + spec->tv_nsec;
  return ret;
}

void init_key(char* key){
  char chars[] = "abcdefghijklmnopqrstuvwxyz1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ!@#$%^&*";
  int i;
  for (i=0; i<5; i++){
    key[i] = chars[rand()%(sizeof(chars)-1)];
    //printf("%c",key[i]);
  }
  key[5] = '\0';
}

int main(int argc, char *argv[]){
  int thread_num=1;
  long iter_num=1;
  char* output_tag = "list";
  char* sync_tag = "-none";
  char yield_tag[5];
  char sync = 'n';
  //Parse options
  while(1){
    int option_index = 0;
    static struct option options[] = {
      {"threads=", required_argument, 0, 0},
      {"iterations=", required_argument, 0, 1},
      {"yield=", required_argument, 0, 2},
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
      opt_yield=0;
      //ADD YIELDOPTS FUNCTIONALITY
      int j;
      //printf("%d\n",(int)strlen(optarg));
      if((int)strlen(optarg)>0){
	for (j=0; j<(int)strlen(optarg); j++){
	  if (optarg[j] == 'i'){
	    opt_yield |= INSERT_YIELD;
	  }
	  else if (optarg[j] == 'd'){
	    opt_yield |= DELETE_YIELD;
	    //printf("d");
	  }
	  else if (optarg[j] == 'l'){
	    opt_yield |= LOOKUP_YIELD;
	  }
	  else{
	    fprintf(stderr, "Unknown yield argument.");
	    _exit(1);
	  }
	}
      }
      //yield_tag="-yield";
      //strcat("-yield", output_tag);
      break;
    case 3:
      if (optarg && strlen(optarg)==1){
	if (optarg[0] == 'm'){ sync = 'm'; sync_tag = "-m";}
	else if (optarg[0] == 's'){ sync = 's'; sync_tag = "-s";}
	//else if (optarg[0] == 'c'){ sync = 'c'; sync_tag = "-c";}
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
  int i;
  //create a pool of linked list elements
  listhead = (SortedList_t*) malloc(sizeof(SortedList_t));
  listhead->next = listhead;
  listhead->prev = listhead;
  listhead->key = NULL;
  pool = (SortedListElement_t*) malloc(thread_num * iter_num * sizeof(SortedListElement_t));

  for (i = 0; i<thread_num*iter_num; i++) {
    //printf("%d",i);
    //printf("%s ",key);
    char* key = (char*) malloc(6*sizeof(char));
    init_key(key);
    //printf("%s\n",key);
    pool[i].key = key;
  }

  //for (i=0; i<thread_num*iter_num; i++) fprintf(stderr, "key %d: %s\n",i,pool[i].key);

  //register segfault handler
  signal(SIGSEGV, catchseg);
  
  //Get start time
  struct timespec start,stop;
  if(clock_gettime(CLOCK_MONOTONIC, &start) == -1){
    fprintf(stderr, "Error getting start time. Reason: %s", strerror(errno));
    _exit(1);
  }
  //printf("%ld\n", start.tv_nsec);
  //Start threading
  pthread_t thread[thread_num];
  //struct thread_args targs;
  //targs.iterations = iter_num;
  //targs.sync = sync;
  
  //initialize mutex
  pthread_mutex_init(&mutexlock, NULL);

  for(i = 0; i<thread_num; i++){
    //char* key = (char*) malloc(6*sizeof(char));
    struct thread_args* targs = (struct thread_args*) malloc(sizeof(struct thread_args));
    targs->iterations=iter_num;
    targs->sync=sync;
    targs->threadNum=i;
    //printf("Thread %d created\n",targs->threadNum);
    if(pthread_create(&thread[i], NULL, add_thread, (void*)targs ) != 0){
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

  int listlength = SortedList_length(listhead);
  if(listlength != 0){
    fprintf(stderr,"List length at end of run is not 0! Exiting.\n");
    _exit(2);
  }
  
  int operations = thread_num*iter_num*3;
  //int time_taken = stop.tv_nsec-start.tv_nsec;
  long time_taken = get_nanosec_from_timespec(&stop) - get_nanosec_from_timespec(&start);
  char output_str[100];
  if (!opt_yield) strcpy(yield_tag, "-none");
  if (opt_yield) strcpy(yield_tag, "-");
  if (opt_yield & INSERT_YIELD) strcat(yield_tag, "i");
  if (opt_yield & DELETE_YIELD) strcat(yield_tag, "d");
  if (opt_yield & LOOKUP_YIELD) strcat(yield_tag, "l");
  
  strcpy(output_str, output_tag);
  strcat(output_str, yield_tag);
  strcat(output_str, sync_tag);
  printf("%s,%d,%ld,1,%d,%ld,%ld\n",output_str,thread_num,iter_num,operations,time_taken,time_taken/operations);

  /*
  printf("%c\n", sync);
  printf("%ld\n", start.tv_nsec);   
  printf("%d\n", thread_num);
  printf("%d\n", iter_num);
  printf("%d\n", opt_yield);
  printf("%ld\n", stop.tv_nsec);
  */
  free(pool);
  return(0);
}
