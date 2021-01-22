//NAME: Darren Lu
//EMAIL: darrenlu3@ucla.edu
//ID: 205394473

#include <getopt.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


void segfault(){
  char* fault = NULL;
  *fault = 1;
}

void catchseg(){
  fprintf(stderr, "My segfault handler, registered by --catch, caught a segfault.\n");
  _exit(4);
}

int main(int argc, char *argv[]){
  //Parsing options
  char* inputfile;
  char* outputfile;
  int c;
  int segflag=0;
  int catchflag=0;
  int inputflag=0;
  int outputflag=0;
  while(1){
    int option_index = 0;
    static struct option options[] = {
      {"input=", required_argument, 0, 0},
      {"output=", required_argument, 0, 1},
      {"segfault", no_argument, 0, 2},
      {"catch", no_argument, 0, 3}
    };
    
    c = getopt_long(argc, argv, "", options, &option_index);
    
    if (c== -1) break;
    switch(c){
    case 0:
      if (optarg && (strlen(optarg)>0)){
	inputflag=1;
	inputfile=optarg;
	//fprintf(stderr,inputfile);
      }
      else{
	fprintf(stderr, "A filename argument is required with the --input= option!\n");
	_exit(1);
      }
      break;
    case 1:
      if (optarg && (strlen(optarg)>0)){
        outputflag=1;
	outputfile=optarg;
	//fprintf(stderr,outputfile);
      }
      else{
        fprintf(stderr, "A filename argument is required with the --\
output= option!\n");
        _exit(1);
      }
      break;
    case 2:
      //segfault();
      segflag=1;
      break;
    case 3:
      //signal(SIGSEGV, catchseg);
      catchflag=1;
      break;
    default:
      fprintf(stderr, "Usage: %s [--input=filename] [--output=filename] [--segfault] [--catch]\n", argv[0]);
      _exit(1);
    }    
  }
  
  //performing input redirection
  if (inputflag){
    int ifd = open(inputfile, O_RDONLY);
    //catch file error
    if (ifd < 0){
      fprintf(stderr, "Could not open %s! --input= caused this error. Reason: %s\n", inputfile, strerror(errno));
      _exit(2);
    }
    else{
      close(0);
      dup(ifd);
      close(ifd);
    }
  }
  if (outputflag){
    int ofd = open(outputfile, O_CREAT | O_RDWR | O_TRUNC, 0644);
    //catch creation error
    if (ofd < 0){
      fprintf(stderr, "Could not create %s! --output= caused this error. Reason: %s\n", outputfile, strerror(errno));
      _exit(3);
    }
    else{
      close(1);
      dup(ofd);
      close(ofd);
    }
  }
  
  
  //register signal handler
  if (catchflag) signal(SIGSEGV, catchseg);
  //cause segfault
  if (segflag) segfault();
  
  int copy;
  while ((copy = getchar()) != EOF){
    putchar(copy);
  }

  return 0;
}
