//NAME: Darren Lu                                           
//EMAIL: darrenlu3@ucla.edu
//ID: 205394473

#include <getopt.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/poll.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <zlib.h>

void duplexio(int fdin, int fdout,int logflag, int compressflag, char* filename){
  //duplex io
  //reading input into a buffer                                                  
  char buffer[256];
  //char logbuf[256];
  char compressbuf[256];
  ssize_t rd;
  int leave = 0,i;
  char crlf[2];
  crlf[0] = 0x0D;
  crlf[1] = 0x0A;
  char intrpt[2];
  intrpt[0] = 0;
  intrpt[1] = 0;
  int lfd;
  if (logflag == 1){
    lfd = open(filename, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (lfd < 0){
      fprintf(stderr, "Could not create logfile %s! Reason: %s",filename, strerror(errno));
      _exit(1);
    }
  }
  
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
  z_stream compressor,decompressor;
  while(numfds > 0){ //main polling loop
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
	if (readfrom == 0){
	  close0=1;
	  close1=1;
	}
	if (readfrom == fds[1].fd){
	  close1=1;
	  close0=1;
	}
      }
      if (rd != 0){
	if (compressflag == 1){
          //initialize compressor                                                             
          compressor.zalloc = Z_NULL;
          compressor.zfree = Z_NULL;
          compressor.opaque = Z_NULL;
          compressor.avail_in = 0;
          compressor.next_in = Z_NULL;
          int compresscode = deflateInit(&compressor, Z_DEFAULT_COMPRESSION);
          if (compresscode != Z_OK){
            fprintf(stderr, "Error occurred while initializing the compressor. Reason: %s", strerror(errno));
            _exit(1);
          }
          //initialize decompressor                                                           
          decompressor.zalloc = Z_NULL;
          decompressor.zfree = Z_NULL;
          decompressor.opaque = Z_NULL;
          decompressor.avail_in =0;
          decompressor.next_in = Z_NULL;
          int decompresscode = inflateInit(&decompressor);
          if (decompresscode != Z_OK){
            fprintf(stderr, "Error occurred while initializing the compressor. Reason: %s", strerror(errno));
            _exit(1);
          }
	  if (readfrom == fdin){ //reading from socket, need to decompress                       
	    memset(compressbuf, 0, sizeof compressbuf);
	    decompressor.avail_in = sizeof buffer;
	    decompressor.next_in = (unsigned char *) buffer;
	    decompressor.avail_out = sizeof compressbuf;
	    decompressor.next_out = (unsigned char*) compressbuf;
	    //decompress message                                                              
	    do {
	      inflate(&decompressor, Z_SYNC_FLUSH);
	    } while(decompressor.avail_in > 0);
	    //move decompressed message to normal buffer                                      
	    //int i;
	    //rd = decompressor.avail_out;
	    printf("Decompressed %zu bytes", rd);
	    /*for (i = 0; i < (int) decompressor.avail_out; i++){
	      //buffer[i] = compressbuf[i];
	      }*/
	  }
	  if (readfrom == 0){ //reading from shell, compressing for transfer          
	    memset(compressbuf, 0, sizeof compressbuf);
	    compressor.avail_in = sizeof buffer;
	    compressor.next_in = (unsigned char*) buffer;
	    compressor.avail_out = sizeof compressbuf;
	    compressor.next_out = (unsigned char*) compressbuf;
	    //compress message                                                               
	    do {
	      deflate(&compressor, Z_SYNC_FLUSH);
	    } while(compressor.avail_in > 0);
	    //move compressed message to normal buffer                                       
	    // int i;
	    printf("Compressed %d bytes", compressor.avail_out);
	    write(fdout, compressbuf, sizeof compressbuf - compressor.avail_out);
	    /*for (i = 0; i < (int) compressor.avail_out; i++){
	      //buffer[i] = compressbuf[i];
	    }*/
	  }
	}
	//fprintf(stderr,"debug:%zd",rd);
	//if(compressflag == 0){
	  for (i = 0; i < rd; i++){
	    switch (buffer[i]){
	    case 0x04:
	      leave=1;
	      intrpt[0] = '^';
	      intrpt[1] = 'D';
	      write(1, intrpt, 2);
	      intrpt[0] = 0x04;
	      if(compressflag == 0) write(fdout, &intrpt[0], 1);
	      break;
	    case 0x03:
	      //fprintf(stderr,"Attempting to kill process %d with signal %d",killid, SIGINT);
	      //kill(killid, SIGINT);
	      //killed = 1;
	      intrpt[0] = '^';
	      intrpt[1] = 'C';
	      write(1, intrpt, 2);
	      intrpt[0] = 0x03;
	      if(compressflag == 0) write(fdout, &intrpt[0], 1);
	      //leave=1;
	      break;
	    case 0x0D:
	    case 0x0A:
	      if(readfrom == fds[1].fd) write(1, crlf, 2);
	      else{
		if(compressflag == 0) write(fdout, &crlf[1], 1);
		write(1, crlf, 2);
	      }
	      break;
	    default:
	      if(readfrom == fds[1].fd) write(1, &buffer[i], 1);
	      else{
		if(compressflag == 0) write(fdout, &buffer[i], 1);
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
	      numfds=0;
	    }
	    else{
	      close(fdin);
	      numfds=0;
	    }
	  }
	  leave = 0;
	  //}
	  if (logflag==1){
	    printf("here");
	    char* logstr="";
	    sprintf(logstr, "SENT %zu bytes: %s", rd, buffer);
	    write(lfd, logstr, strlen(logstr));
	  }
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
      //close(fdout);
      fdoutclosed = 1;
      numfds--;
    }
    if(fdoutclosed == 1){
      //close(fdin);
      numfds--;
    }
    if(close1 == 1){
      //close the pipe from the shell to the process
      //close(fdin);
      numfds--;
    }
  }
  /*
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
  */
  return;
}

void connecttoserver(int port,int logflag, int compressflag, char* filename){
  //getting terminal mode and making a backup
  struct termios tsettings, tbackup;
  
  int tcget = tcgetattr(0, &tsettings);
  if (tcget != 0){
    fprintf(stderr, "Could not complete tcgetattr. Reason: %s", strerror(errno));
    _exit(1);
  }
  else{
    tbackup = tsettings;
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

  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0){
    fprintf(stderr, "Error occurred creating socket.");
    _exit(1);
  }

  char *host = "localhost";
  struct hostent *server =gethostbyname(host);
  //construct sockadrr_in struct
  struct sockaddr_in serv_addr;
  bzero((char*) &serv_addr, sizeof(serv_addr));
  bcopy((char*) server->h_addr, (char*) &serv_addr.sin_addr.s_addr, server->h_length);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);

  int connectcode = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)); 
  if (connectcode < 0){
    fprintf(stderr, "Error occurred while connecting socket to remote host. Reason: %s\n", strerror(errno));
    _exit(1);
  };

  //printf("Connected!\n");

  duplexio(sockfd, sockfd,logflag,compressflag, filename);
  //fprintf(stderr,"Socket closed by server\n");
  /*
  int shutdowncode = shutdown(sockfd, SHUT_RDWR);
  if (shutdowncode < 0){
    fprintf(stderr, "Error occured while closing socket. Reason: %s\n", strerror(errno));
    _exit(1);
  }
  */

  //restoring normal terminal modes
  tcset = tcsetattr(0, TCSANOW, &tbackup);
  if (tcset != 0){
    fprintf(stderr, "Could not complete tcsetattr. Reason: %s\n", strerror(errno));
    _exit(1);
  }
  else{
    //printf("termios set success!\n");
  }

  return;

}

int main(int argc, char *argv[]){
  //parsing options
  int logflag=0,compressflag=0, c, port=2000;
  char* logfile;
  while(1){
    int option_index = 0;
    static struct option options[] = {
      {"port=", required_argument, 0, 0},
      {"log=", required_argument, 0, 1},
      {"compress", no_argument, 0, 2}
    };
    c = getopt_long(argc, argv, "", options, &option_index);
    if (c == -1) break;
    switch (c){
    case 0:
      if (optarg && (strlen(optarg)>0)){
	port=atoi(optarg); //set the port
      }
      else{
	fprintf(stderr, "A port must be specified for the --port= option!\n");
	_exit(1);
      }
      break;
    case 1:
      logflag=1;
      logfile=optarg;
      break;
    case 2:
      compressflag=1;
      break;
    default:
      fprintf(stderr, "Usage: %s [--port=port [--log=filename] [--compress]\n", argv[0]);
      _exit(1);
      break;
    }
  }
  if (logflag != 1){
    logfile = '\0';
  }
  connecttoserver(port,logflag,compressflag,logfile);
  //printf("%d",port);
  return(0);
}
