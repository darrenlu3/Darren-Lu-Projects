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
//long *wait_time;
pthread_mutex_t* locks;
SortedList_t** listheads;
int numlists = 1;

void catchseg(){
  fprintf(stderr, "My segfault handler caught a segfault.\n");
  _exit(2);
}

static inline unsigned long get_nanosec_from_timespec(struct timespec* spec)
//function from discussion 1b week 4
{
  unsigned long ret = spec->tv_sec;
  ret = ret * 1000000000 + spec->tv_nsec;
  return ret;
}

int hashkey(const char* key){
  int hash=0;
  unsigned int i=0;
  //fprintf(stderr,"entered hash");
  for (i = 0; i < strlen(key); i++){
    hash += (int)key[i];
  }
  hash = hash%numlists; 
  //fprintf(stderr,"hash number %d\n",hash);
  return hash;
}

void Mul_SortedList_insert(SortedListElement_t *element){
  int list_num = hashkey(element->key);
  pthread_mutex_lock(&locks[list_num]);
  SortedList_insert(listheads[list_num], element);
  //fprintf(stderr,"inserted %s into list %d ", element->key, list_num);
  //fprintf(stderr,"list %d first element %s \n", list_num, listheads[list_num]->next->key);
  pthread_mutex_unlock(&locks[list_num]);
}

SortedListElement_t* Mul_SortedList_lookup(const char* key){
  SortedListElement_t* ret = NULL;
  int list_num = hashkey(key);
  //fprintf(stderr,"finding %s in list %d\n", key, list_num);
  //fprintf(stderr,"list %d %s\n",list_num, listheads[list_num]->next->key);
  pthread_mutex_lock(&locks[list_num]);
  ret = SortedList_lookup(listheads[list_num], key);
  pthread_mutex_unlock(&locks[list_num]);
  return ret;
}

int Mul_SortedList_length(void){
  int i=0, length=0;
  for (i=0;i<numlists;i++){
    pthread_mutex_lock(&locks[i]);
    length += SortedList_length(listheads[i]);
    pthread_mutex_unlock(&locks[i]);
  }
  return length;
}

struct thread_args{
  int threadNum;
  int iterations;
  char sync;
  long time_waited;
};
long lock = 0;
long timewaited;
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
  //FUNCTIONALITY with help from discussion 1b week 4 and 5

  //Get start time                                                                                                  
  struct timespec lockstart,lockstop;
  if(clock_gettime(CLOCK_MONOTONIC, &lockstart) == -1){
    fprintf(stderr, "Error getting start time. Reason: %s", strerror(errno));
    _exit(1);
  }

  if (mode == 'm') pthread_mutex_lock(&mutexlock);
  if (mode == 's') while(__sync_lock_test_and_set(&lock,1));

  //Get stop time
  if(clock_gettime(CLOCK_MONOTONIC, &lockstop) == -1){
    fprintf(stderr, "Error getting stop time. Reason: %s", strerror(errno));
    _exit(1);
  } 
  //fprintf(stderr, "threadnum %d", threadNum);
  //wait_time[threadNum] = (long) malloc(sizeof(long));
  timewaited = get_nanosec_from_timespec(&lockstop) - get_nanosec_from_timespec(&lockstart);
  //fprintf(stderr, "%ld\n", timewaited);

  //for(i = startIndex; i< startIndex + iter; i++) SortedList_insert(listhead, &pool[i]);
  for(i=startIndex; i<startIndex + iter; i++) Mul_SortedList_insert(&pool[i]);

  //int status = SortedList_length(listhead);
  int status = Mul_SortedList_length();
  if (status <= 0){
    fprintf(stderr,"SortedList_length returned a nonpositive value. Exiting.\n");
    _exit(2);
  }
  //fprintf(stderr, "Thread %d sorted list length %d\n",threadNum,status);
  SortedListElement_t* e;
  for(i = startIndex; i<startIndex + iter; i++){
    //e = SortedList_lookup(listhead, (&pool[i])->key);
    e = Mul_SortedList_lookup((&pool[i])->key);
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

  return((void*)&timewaited);
  pthread_exit(NULL);
}


void init_key(char* key){
  char chars[] = "abcdefghijklmnopqrstuvwxyz1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ!@#$%^&*";
  int i;
  for (i=0; i<5; i++){
    key[i] = chars[rand()%(sizeof(chars)-1)];
    //printf("%c",key[i].);
  }
  key[5] = '\0';
}

int main(int argc, char *argv[]){
  int thread_num=1;
  int listflag=1;
  long* wait_time;
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
      {"sync=", required_argument, 0, 3},
      {"lists=", required_argument, 0, 4}
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
    case 4:
      if (optarg){
	listflag= atoi(optarg);
	numlists=listflag;
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

  //set up multiple linked lists
  
  listheads = (SortedList_t**) malloc(listflag * sizeof(SortedList_t*));
  for(i = 0; i<listflag; i++){
    listheads[i] = (SortedList_t*)(malloc(sizeof(SortedList_t*)));
    listheads[i]->next = listhead;
    listheads[i]->prev = listhead;
    listheads[i]->key = NULL;
    }
  //for (i=0; i<thread_num*iter_num; i++) fprintf(stderr, "key %d: %s\n",i,pool[i].key);

  //register segfault handler
  signal(SIGSEGV, catchseg);
  
  //set up wait time array
  wait_time = (long*) calloc(thread_num, sizeof(long));

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
  //initialize mutex locks for multi lists
  locks = (pthread_mutex_t*) malloc(listflag * sizeof(pthread_mutex_t));
  for(i = 0; i<listflag; i++){
    pthread_mutex_init(&(locks[i]), NULL);
  }

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
  void* locktime;
  for(i = 0; i<thread_num; i++){
    if(pthread_join(thread[i], &locktime) != 0){
      fprintf(stderr, "Error joining thread number %d. Reason: %s", i+1, strerror(errno));
      _exit(1);
    }
    wait_time[i]=*(long*)locktime;
  }
  
  
  //destroy mutex
  pthread_mutex_destroy(&mutexlock);

  //Get stop time
  if(clock_gettime(CLOCK_MONOTONIC, &stop) == -1){
    fprintf(stderr, "Error getting stop time. Reason: %s", strerror(errno));
    _exit(1);
  }

  unsigned long lock_time = 0;
  //calculate total lock acquisition time
  for (i = 0; i<thread_num; i++){
    //fprintf(stderr,"%ld\n",wait_time[i]);
    lock_time += wait_time[i];
    //fprintf(stderr,"%ld\n",lock_time);
  }

  int listlength = SortedList_length(listhead);
  if(listlength != 0){
    fprintf(stderr,"List length at end of run is not 0! Exiting.\n");
    _exit(2);
  }
  
  int operations = thread_num*iter_num*3;
  lock_time=lock_time/operations;

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
  printf("%s,%d,%ld,%d,%d,%ld,%ld,%ld\n",output_str,thread_num,iter_num,listflag,operations,time_taken,time_taken/operations,lock_time);

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
