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
#include <zlib.h>

void duplexio(int fdin, int fdout, pid_t killid, int sockfd, int compressflag){
  //duplex io
  //reading input into a buffer                                                  
  char buffer[256],compressbuf[256];
  //char servtoclient[256];
  ssize_t rd;
  int leave = 0,i;
  char crlf[2];
  crlf[0] = 0x0D;
  crlf[1] = 0x0A;
  //char intrpt[2];
  //intrpt[0]=0;
  //intrpt[1]=0;

  
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
	if (readfrom == fds[1].fd) close1=1;
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
	  
	  if (readfrom == 0){ //reading from socket, need to decompress
	    fprintf(stderr, "Reading from socket\n");
	    //memset(compressbuf, 0, sizeof compressbuf);
	    fprintf(stderr, "1");
	    decompressor.avail_in = rd;
	    decompressor.next_in = (unsigned char *) buffer;
	    decompressor.avail_out = sizeof compressbuf;
	    decompressor.next_out = (unsigned char*) compressbuf;
	    //decompress message
	    do {
	      inflate(&decompressor, Z_SYNC_FLUSH);
	    } while(decompressor.avail_in > 0);
	    fprintf(stderr, "2");
	    //move decompressed message to normal buffer
	    int i;
	    rd = decompressor.avail_out;
	    printf("Decompressed %zu bytes", rd);
	    for (i = 0; i < (int)decompressor.avail_out; i++){
	      buffer[i] = compressbuf[i];
	    }
	  }
	  if (readfrom == fds[1].fd){ //reading form shell, compressing for transfer
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
            //int i;
            //rd = compressor.avail_out;
	    printf("Compressed %zu bytes", rd);
            /*for (i = 0; i < (int)compressor.avail_out; i++){
              buffer[i] = compressbuf[i];
	      }*/
	  }	  
	}

	//fprintf(stderr,"debug:%zd",rd);
	for (i = 0; i < rd; i++){
	  switch (buffer[i]){
	  case 0x04:
	    leave=1;
            //intrpt[0] = '^';
            //intrpt[1] = 'D';
	    //write(1, intrpt, 2);
	    //intrpt[0] = 0x04;
	    //write(fdout, &intrpt[0], 1);
	    break;
	  case 0x03:
	    //fprintf(stderr,"Attempting to kill process %d with signal %d",killid, SIGINT);
	    kill(killid, SIGINT);
	    //killed = 1;
	    //intrpt[0] = '^';
	    //intrpt[1] = 'C';
	    //write(1, intrpt, 2);
	    //leave=1;
	    break;
	  case 0x0D:
	  case 0x0A:
	    if(readfrom == fds[1].fd){
	      write(sockfd, &crlf[1], 1);
	      //write(1, crlf, 2);
	    }
	    else{
	      write(fdout, &crlf[1], 1);
	      //write(1, crlf, 2);
	    }
	    break;
	  default:
	    if(readfrom == fds[1].fd){
	      //write(1, &buffer[i], 1);
	      //servtoclient[i] = buffer[i];
	      write(sockfd, &buffer[i], 1);
	    }
	    else{
	      write(fdout, &buffer[i], 1);
	      //write(1, &buffer[i], 1);
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

    //if(readfrom != -1) write(sockfd, servtoclient, rd);
    //servtoclient[0]='\0';

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
    if(compressflag == 1){
      deflateEnd(&compressor);
      inflateEnd(&decompressor);
    }
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
  
  return;
}

void connecttoclient(int port, int compressflag){
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0){
    fprintf(stderr, "Error occurred creating socket.");
  }

  //construct sockadrr_in struct
  struct sockaddr_in serv_addr;
  bzero((char*) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);
  
  int bindcode = bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
  if (bindcode < 0){
    fprintf(stderr, "Error occured while binding name to socket. Reason: %s\n",strerror(errno));
    _exit(1);
  }

  int listencode = listen(sockfd, 5);
  if (listencode == 1){
    fprintf(stderr, "Error occured while listening to socket. Reason: %s\n",strerror(errno));
    _exit(1);
  }

  struct sockaddr client_addr;
  bzero((char*) &client_addr, sizeof(client_addr));
  unsigned int addrlength = sizeof(client_addr);
  int acceptfd = accept(sockfd, (struct sockaddr *) &client_addr, &addrlength);
  if (acceptfd < 0){
    fprintf(stderr, "Error occured while accepting socket. Reason: %s\n",strerror(errno));
    _exit(1);
  }
  
  //printf("Accepted!\n");

  //SHELL routine from project 1a
  //create pipes for io
  int proctoshell[2];
  int shelltoproc[2];

  if ((pipe(proctoshell) != 0) || (pipe(shelltoproc) != 0)){
    fprintf(stderr, "Error occurred while creating pipes. Reason: %s", strerror(errno));
  }
    
  //fork a child
  int pid = fork();
  if (pid < 0){
    fprintf(stderr, "Fork failed. Reason: %s",strerror(errno));
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
      fprintf(stderr,"Error creating shell with execlp. Reason: %s\n", strerror(errno));
      _exit(1);
    }
  }
  else{
    //redirect socket fd to stdin
    close(0);
    dup(acceptfd);
    //close(acceptfd);
    //setting up unidirectional pipes
    close(shelltoproc[1]);
    close(proctoshell[0]);
    duplexio(shelltoproc[0],proctoshell[1], pid, acceptfd, compressflag);
  }

  int shutdowncode = shutdown(sockfd, SHUT_RDWR);
  if (shutdowncode < 0){
    fprintf(stderr, "Error occured while closing socket. Reason: %s\n",strerror(errno));
    _exit(1);
  }

  return;

}

int main(int argc, char *argv[]){
  //parsing options
  int compressflag=0, c, port=2000;
  while(1){
    int option_index = 0;
    static struct option options[] = {
      {"port=", required_argument, 0, 0},
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
    case 2:
      compressflag=1;
      break;
    default:
      fprintf(stderr, "Usage: %s [--port=port [--log=filename] [--compress]\n", argv[0]);
      _exit(1);
      break;
    }
  }

  if (compressflag == 1){

  }
  connecttoclient(port, compressflag);
  //printf("%d",port);
  return (0);
}
