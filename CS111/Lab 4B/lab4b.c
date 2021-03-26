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

#define B 4275
#define R0 100000.0

int period = 1, logflag = 0, logfd = 1, stopflag = 0,started=0;
char scale = 'F';
float temperature;
mraa_aio_context sensor;
mraa_gpio_context button;

void print_curr_time(){
    if (stopflag == 1) return; //STOP logging
    struct timespec ts;
    struct tm* tm;
    clock_gettime(CLOCK_REALTIME, &ts);
    tm = localtime(&(ts.tv_sec));
    char* output = (char*)malloc(8*sizeof(char));
    sprintf(output, "%02d:%02d:%02d", tm->tm_hour, tm->tm_min, tm->tm_sec);
    printf(output);
    //printf("%02d:%02d:%02d", tm->tm_hour, tm->tm_min, tm->tm_sec);
    if (logflag == 1){
        write(logfd, output, strlen(output));
    }
}

void shutdown(){
    //print time
    stopflag = 0; //enable time printing again
    print_curr_time();
    printf(" SHUTDOWN\n");
    if (logflag == 1){
        //log to file
        write(logfd, " SHUTDOWN\n", 10);
        close(logfd);
    }
    mraa_aio_close(sensor);
    mraa_gpio_close(button);
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
    print_curr_time();
    char* output = (char*)malloc(6*sizeof(char));
    //printf(" %.1f\n", read_temp(rawreading));
    sprintf(output, " %.1f\n", read_temp(rawreading));
    printf(output);
    if (logflag == 1){
        write(logfd, output, strlen(output));
    }
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
        shutdown();
    }
    if (putlog == 1 && logflag == 1){
        write(logfd, command, strlen(command));
        write(logfd, "\n", 1);
    }
}

void sensor_init(){
    //initialize MRAA pin 60
    button = mraa_gpio_init(60);
    //set it to be input
    mraa_gpio_dir(button, MRAA_GPIO_IN);
    //set up button shutdown
    mraa_gpio_isr(button, MRAA_GPIO_EDGE_RISING, &shutdown, NULL);
    
    //set up analog temp reading
    sensor = mraa_aio_init(1);
}
           
int main(int argc, char *argv[]){
    //Parsing options
    int c;
    while (1){
        int option_index = 0;
        static struct option options[] = {
            {"period=", required_argument, 0, 0},
            {"scale=", required_argument, 0, 1},
            {"log=", required_argument, 0, 2}
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
            default:
                fprintf(stderr, "Usage: ./lab4b [--period=period] [--scale=F or C] [--log=filename]");
                _exit(1);
        }
    }
    
    //initialize snesors
    sensor_init();
    
    //setup up polling
    struct pollfd pollStdin = {0, POLLIN, 0};
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
            int rd = read(0, buf, 128);
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
}
