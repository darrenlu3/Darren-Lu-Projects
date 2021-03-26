#ifdef DUMMY
#define MRAA_GPIO_IN 0
typedef int mraa_aio_context;
typedef int mraa_gpio_context;
//dummy functions for GPIO
mraa_aio_context mraa_aio_init(int p){
    return 0;
}
mraa_aio_read(mraa_aio_context c){
    return 650;
}
void mraa_aio_close(mraa_aio_context c){
}
mraa_gpio_context mraa_gpio_init(int p){
    return 0;
}
void mraa_gpio_dir(mraa_gpio_context c, int d){
}
int mraa_gpio_read(mraa_gpio_context c){
    return 0;
}
void mraa_gpio_close(mraa_gpio_context c){
}
#else
#include <mraa.h>
#include <mraa/aio.h>
#include <mraa/gpio.h>
#endif

#include <signal.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <getopt.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <netdb.h>
#include <openssl/ssl.h>

#define B 4275
#define R0 100000.0

int period = 1, logflag = 0, logfd = 1, stopflag = 0, started=0, hostgiven = 0, idgiven = 0, id, portnumber, infd = 0, outfd = 1;
SSL *sslClient;
char scale = 'F';
char* hostname;
float temperature;
mraa_aio_context sensor;
mraa_gpio_context button;

void ssl_clean_client(SSL* client){
    SSL_shutdown(client);
    SSL_free(client);
}

char* print_curr_time(){
    if (stopflag == 1) return ""; //STOP logging
    struct timespec ts;
    struct tm* tm;
    clock_gettime(CLOCK_REALTIME, &ts);
    tm = localtime(&(ts.tv_sec));
    char* output = (char*)malloc(8*sizeof(char));
    sprintf(output, "%02d:%02d:%02d", tm->tm_hour, tm->tm_min, tm->tm_sec);
//    //printf(output);
//    write(outfd, output, strlen(output));
//    //printf("%02d:%02d:%02d", tm->tm_hour, tm->tm_min, tm->tm_sec);
//    if (logflag == 1){
//        write(logfd, output, strlen(output));
//    }
    return(output);
}

void myshutdown(){
    //print time
    stopflag = 0; //enable time printing again
    char*output = (char*)malloc(18*sizeof(char));
    sprintf(output, "%s SHUTDOWN\n", print_curr_time());
    SSL_write(sslClient, output, 18);
    //printf(" SHUTDOWN\n");
    if (logflag == 1){
        //log to file
        write(logfd, output, 18);
        close(logfd);
    }
    mraa_aio_close(sensor);
    mraa_gpio_close(button);
    ssl_clean_client(sslClient);
    _exit(0);
}

float read_temp(int raw){
    float R = 1023.0/((float) raw) - 1.0;
    R = R0*R;
    float result = 1.0/ (log(R/R0) /B+1/298.15) - 273.15;
    if (scale == 'C') return result;
    return result*1.8 + 32;
}

void print_temp(mraa_aio_context sensor){
    if (stopflag == 1) return; //STOP logging
    int rawreading = mraa_aio_read(sensor);
    char* output = (char*)malloc(15*sizeof(char));
    //printf(" %.1f\n", read_temp(rawreading));
    sprintf(output, "%s %.1f\n", print_curr_time(), read_temp(rawreading));
    //printf(output);
    if (logflag == 1){
        write(logfd, output, strlen(output));
    }
    SSL_write(sslClient, output, strlen(output));
}

void runtime_command(char* command){
    int putlog = 0;
    if(strcmp(command, "SCALE=F") == 0){
        scale = 'F';
        putlog = 1;
    }
    else if (strcmp(command, "SCALE=C") == 0){
        scale = 'C';
        putlog = 1;
    }
    else if (strncmp(command, "PERIOD=", 7) == 0){
        period = atoi(command+7);
        putlog = 1;
    }
    else if (strcmp(command, "STOP") == 0){
        stopflag = 1;
        putlog = 1;
        started=0;
    }
    else if (strcmp(command, "START") == 0){
        stopflag = 0;
        putlog = 1;
        started = 0;
    }
    else if (strncmp(command, "LOG", 3) == 0){
        putlog = 1;
    }
    else if (strcmp(command, "OFF") == 0){
        write(logfd, command, strlen(command));
        write(logfd, "\n", 1);
        myshutdown();
    }
    if (putlog == 1 && logflag == 1){
        write(logfd, command, strlen(command));
        write(logfd, "\n", 1);
    }
}

void sensor_init(){
    //initialize MRAA pin 60
    //button = mraa_gpio_init(60);
    //set it to be input
    //mraa_gpio_dir(button, MRAA_GPIO_IN);
    //set up button shutdown
    //mraa_gpio_isr(button, MRAA_GPIO_EDGE_RISING, &myshutdown, NULL);
    
    //set up analog temp reading
    sensor = mraa_aio_init(1);
}

//from discussion
int client_connect(char* host_name, unsigned int port){
    struct sockaddr_in serv_addr;
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0){
        fprintf(stderr, "Could not create socket! Reason: %s\n", strerror(errno));
        _exit(1);
    }
    struct hostent *server = gethostbyname(host_name);
    //converting host_name to ip address
    memset(&serv_addr,0,sizeof(struct sockaddr_in));
    serv_addr.sin_family = AF_INET;
    memcpy(&serv_addr.sin_addr.s_addr,server->h_addr,server->h_length);
    serv_addr.sin_port = htons(port);
    int status = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
    if (status < 0){
        fprintf(stderr, "Could not connect to given host! Reason: %s\n", strerror(errno));
        _exit(1);
    }
    return sockfd;
}

SSL_CTX * ssl_init(void){
    //initialization of ssl
    SSL_CTX* newContext = NULL;
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
    newContext = SSL_CTX_new(TLSv1_client_method());
    
    return newContext;
}

SSL* attach_ssl_to_socket(int socket, SSL_CTX *context){
    SSL* mysslClient = SSL_new(context);
    SSL_set_fd(mysslClient,socket);
    SSL_connect(mysslClient);
    return mysslClient;
}

int main(int argc, char *argv[]){
    //Parsing options
    int c;
    int option_index;
    while (1){
        option_index = 0;
        static struct option options[] = {
            {"period=", required_argument, 0, 0},
            {"scale=", required_argument, 0, 1},
            {"log=", required_argument, 0, 2},
            {"id=", required_argument, 0, 3},
            {"host=", required_argument, 0, 4},
            {0, no_argument, 0, 0}
        };
        
        c = getopt_long(argc, argv, "", options, &option_index);
        
        if (c == -1) break;
        switch(c){
            case 0:
                //period
                period = atoi(optarg);
                break;
            case 1:
                //scale
                if (strcmp("F",optarg)==0) scale = 'F';
                else if (strcmp("C",optarg)==0) scale = 'C';
                else{
                    fprintf(stderr, "Wrong scale given!\n");
                    _exit(1);
                }
                break;
            case 2:
                //log
                logflag = 1;
                logfd = open(optarg, O_CREAT | O_RDWR | O_TRUNC, 0644);
                if (logfd < 0){
                    fprintf(stderr, "Could not write to %s! Reason: %s\n", optarg, strerror(errno));
                    _exit(1);
                }
                break;
            case 3:
                //id
                idgiven = 1;
                id = (atoi(optarg));
                break;
            case 4:
                //host
                hostgiven = 1;
                hostname = optarg;
                break;
            default:
                fprintf(stderr, "Usage: ./lab4b --id=(9 digit id) --host=(name or address) --log=filename PORTNUMBER [--period=period] [--scale=F or C]");
                _exit(1);
        }
    }
    
    if (idgiven == 0){
        fprintf(stderr, "No id given!\n");
        _exit(1);
    }
    else{
        int numdigits = floor(log10(abs(id))) + 1;
        if (numdigits != 9){
            fprintf(stderr, "Incorrect id length given!\n");
            _exit(1);
        }
    }
    if (hostgiven == 0){
        fprintf(stderr, "No host given!\n");
        _exit(1);
    }
    if (logflag == 0){
        fprintf(stderr, "No log file provided!\n");
        _exit(1);
    }
    if (optind >= argc){
        fprintf(stderr, "No port number given!\n");
        _exit(1);
    }
    else{
        portnumber = atoi(argv[optind]);
    }
    
    //printf("REMOVE THIS. TESTING. id = %d host = %s port = %d\n",id,hostname,portnumber);
    
    int sockfd = client_connect(hostname, portnumber);
    SSL_CTX* context = ssl_init();
    sslClient= attach_ssl_to_socket(sockfd, context);
    
    
    char strid[14];
    sprintf(strid, "ID=%d\n", id);
    write(logfd, strid, strlen(strid));
    SSL_write(sslClient, strid, strlen(strid));
    
    //initialize snesors
    sensor_init();
    
    //setup up polling
    struct pollfd pollStdin = {sockfd, POLLIN, 0};
    char buf[128], command[128];
    
    
    //begin runtime
    //printf("scale %c, logfd %d, period %d\n", scale, logfd, period);
    int ret=0;
    time_t output_time = time(NULL);

    while(1){
        //report temperature
        time_t now = time(NULL);
        if(started == 1 && (now == output_time + period)){
            print_temp(sensor);
            output_time = now;
        }
        else if (started == 0){
            print_temp(sensor);
            output_time = now;
            started = 1;
        }
        
        //poll and see if there is input from stdin
        ret = poll(&pollStdin,1,1000);
        if (ret == -1){
            fprintf(stderr, "An error occured while polling. %s\n", strerror(errno));
            _exit(1);
        }
        else if (ret >= 1){
            int i,j = 0;
            memset(command, 0, 128);
            memset(buf, 0, 128);
            int rd = SSL_read(sslClient, buf, 128);
            for (i = 0; i < rd; i++){
                if((char)buf[i] == '\n'){
                    //send command to be handled
                    //printf("received command: %s\n",command);
                    runtime_command(command);
                    j = 0;
                    memset(command, 0, 128);
                }
                else{
                    command[j] = (char)buf[i];
                    j++;
                }
            }
        }
    }
    if (logflag == 1){
        close(logfd);
    }
    ssl_clean_client(sslClient);
}
