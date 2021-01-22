
//NAME: Darren Lu                                           
//EMAIL: darrenlu3@ucla.edu
//ID: 205394473

#include <getopt.h>
#include <termios.h>
#include <unistd.h>
#include <getopt.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/poll.h>
#include <signal.h>


void duplexio(int forked, int fdin, int fdout, pid_t killid){
  //getting terminal mode and making a backup
  struct termios tsettings, tbackup;
  
  int tcget = tcgetattr(0, &tsettings);
  if (tcget != 0){
    fprintf(stderr, "Could not complete tcgetattr. Reason: %s", strerror(errno));
    _exit(1);
  }
  else{
    //printf("termios get success!\n");	  
    tbackup = tsettings;
    //fprintf(stdout, "tcget value: %d\n", tcget);
  }

  //setting terminal mode
  tsettings.c_iflag = ISTRIP;
  tsettings.c_oflag = 0;
  tsettings.c_lflag = 0;
  
  int tcset = tcsetattr(0, TCSANOW, &tsettings);
  if (tcset != 0){
    fprintf(stderr, "Could not complete tcsetattr. Reason: %s", strerror(errno));
    _exit(1);
  }
  else{
    //printf("termios set success!\n");
  }

  //duplex io
  //reading input into a buffer                                                  
  char buffer[256];
  ssize_t rd;
  int leave = 0,i;
  char crlf[2];
  crlf[0] = 0x0D;
  crlf[1] = 0x0A;
  char intrpt[2];
  if (forked == 1){
    struct pollfd fds[2];
    int readfrom,close0 = 0,close1 = 0;
    fds[0].fd = 0;
    fds[0].events = POLLIN;
    fds[1].fd = fdin;
    fds[1].events = POLLIN;
    int numfds = 2;
    int fdoutclosed = 0, killed = 0;
    //register a sigpipe handler
    //signal(SIGPIPE, catchpipe);

    while(numfds != 0){ //main polling loop
      int result = poll(fds, 2, 0);
      if (result == -1){
	fprintf(stderr, "An error occured while polling. %s\n", strerror(errno));
	_exit(1);
      }
      if(fds[0].revents & POLLIN){
	readfrom = fds[0].fd;
      }
      else if(fds[1].revents & POLLIN){
	readfrom = fds[1].fd;
      }
      else readfrom = -1;
      if (readfrom != -1){ rd = read(readfrom, buffer, 255);
      if (rd == 0){
	//fprintf(stderr,"EOF on file descriptor %d",readfrom);
	if (readfrom == 0) close0=1;
	if (readfrom == fds[1].fd) close1=1;
      }
      if (rd != 0){
	//fprintf(stderr,"debug:%zd",rd);
	for (i = 0; i < rd; i++){
	  switch (buffer[i]){
	  case 0x04:
	    leave=1;
            intrpt[0] = '^';
            intrpt[1] = 'D';
	    write(1, intrpt, 2);
	    //intrpt[0] = 0x04;
	    //write(fdout, &intrpt[0], 1);
	    break;
	  case 0x03:
	    //fprintf(stderr,"Attempting to kill process %d with signal %d",killid, SIGINT);
	    kill(killid, SIGINT);
	    //killed = 1;
	    intrpt[0] = '^';
	    intrpt[1] = 'C';
	    write(1, intrpt, 2);
	    //leave=1;
	    break;
	  case 0x0D:
	  case 0x0A:
	    if(readfrom == fds[1].fd) write(1, crlf, 2);
	    else{
	      write(fdout, &crlf[1], 1);
	      write(1, crlf, 2);
	    }
	    break;
	  default:
	    if(readfrom == fds[1].fd) write(1, &buffer[i], 1);
	    else{
	      write(fdout, &buffer[i], 1);
	      write(1, &buffer[i], 1);
	    }
	    break;
	  }
	  if (leave == 1) break;
	}
	if (killed == 1){
	  close(fdout);
	  close(fdin);
	  numfds = 0;
	  break;
	}
	//shutdown processing for EOF
	if (leave == 1){
	  if(readfrom == fds[0].fd){
	    //close the pipe to the shell
	    close(fdout);
	    numfds--;
	  }
	  else{
	    close(fdin);
	    numfds--;
	  }
	}
	leave = 0;
      }
      }
      if(fds[0].revents & (POLLHUP | POLLERR)){
	close0 = 1;
      }
      if(fds[1].revents & (POLLHUP | POLLERR)){
	close1 = 1;
      }
      if(close0 == 1){
	//close the pipe to the shell                                          
	close(fdout);
	fdoutclosed = 1;
	numfds--;
      }
      if(fdoutclosed == 1){
	close(fdin);
	numfds--;
      }
      if(close1 == 1){
	//close the pipe from the shell to the process
	close(fdin);
	numfds--;
      }
    }

    //escape sequence detected, restoring normal terminal modes                      
    tcset = tcsetattr(0, TCSANOW, &tbackup);
    if (tcset != 0){
      fprintf(stderr, "Could not complete tcsetattr. Reason: %s", strerror(errno)\
	      );
      _exit(1);
    }
    else{
      //printf("termios set success!\n");                                            
    }

    int status,loworder,highorder;
    pid_t exitid = waitpid(killid, &status, WUNTRACED | WCONTINUED);
    if(exitid == -1){
      fprintf(stderr,"Error occurred while waiting for child process %d", killid);
      _exit(1);
    }
    if (WIFEXITED(status)){
      highorder = WEXITSTATUS(status);
    }
    loworder = status & 0x007f;
    //highorder = status & 0xff00;
    fprintf(stderr, "\nSHELL EXIT SIGNAL=%d STATUS=%d\n",loworder,highorder);
  }
  else{
    //reading input into a buffer
    while ((rd = read(0, buffer, 255)) != 0){
      //write(1, buffer, rd);
      //processing
      for (i = 0; i < rd; i++){
	switch (buffer[i]){
	case 0x04:
	  leave=1;
	  break;
	case 0x0D:
	case 0x0A:
	  write(1, crlf, 2);
	  break;
	default:
	  write(1, &buffer[i], 1);
	  break;
	}
	if (leave == 1) break;
      }
      if (leave == 1) break;
    }
  }

  //escape sequence detected, restoring normal terminal modes
  tcset = tcsetattr(0, TCSANOW, &tbackup);
  if (tcset != 0){
    fprintf(stderr, "Could not complete tcsetattr. Reason: %s", strerror(errno)\
);
    _exit(1);
  }
  else{
    //printf("termios set success!\n");
  }

  return;
}

int main(int argc, char *argv[]){
  //parsing options
  int shellflag=0, c;
  while(1){
    int option_index = 0;
    static struct option options[] = {
      {"shell", no_argument, 0, 0}};
    c = getopt_long(argc, argv, "", options, &option_index);
    if (c == -1) break;
    switch (c){
    case 0:
      shellflag=1;
      break;
    default:
      fprintf(stderr, "Usage: %s [--shell]\n", argv[0]);
      _exit(1);
      break;
    }
  }

  if (shellflag == 1 ){
    //create pipes for io
    int proctoshell[2];
    int shelltoproc[2];

    if ((pipe(proctoshell) != 0) || (pipe(shelltoproc) != 0)){
      fprintf(stderr, "Error occurred while creating pipes. Reason: %s", strerror(errno));
    }
    
    //fork a child
    int pid = fork();
    if (pid < 0){
      fprintf(stderr, "fork failed");
      _exit(1);
    }
    else if (pid == 0){
      //setting up unidirectional pipes
      close(proctoshell[1]);
      close(shelltoproc[0]);
      //redirecting shell stdin
      close(0);
      dup(proctoshell[0]); //proctoshell read should be on fd 0 now
      close(proctoshell[0]);
      //redirecting shell stdout and stderr
      close(1);
      dup(shelltoproc[1]);
      close(2);
      dup(shelltoproc[1]); //shelltoproc write should be fd 1 and 2 now
      close(shelltoproc[1]);
      //call the shell
      int shell = execlp("/bin/bash", "/bin/bash", (char*)NULL);
      if (shell == -1){
	fprintf(stderr,"Error creating shell with execlp");
	_exit(1);
      }
    }
    else{
      //setting up unidirectional pipes
      close(shelltoproc[1]);
      close(proctoshell[0]);
      duplexio(1,shelltoproc[0],proctoshell[1], pid); //set up channels to forward ascii input to shell
    }
  }
  else duplexio(0,0,1,0);

  return(0);
}
