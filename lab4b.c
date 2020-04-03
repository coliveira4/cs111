//NAME: Christina Oliveira
//EMAIL: christina.oliveira.m@gmail.com
//ID: 204803448

#include <time.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <math.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <mraa.h>
#include <mraa/aio.h>
#include <mraa/gpio.h>
#define BUFSIZE 256
#define STDIN 0

sig_atomic_t volatile sampleFlag = 1;
int isStopped = 0;
const int B = 4275;
const int R0 = 100000;
time_t raw_time;
struct tm* current_time;
mraa_gpio_context button;
mraa_aio_context tempSensor;
int logFd = -1;

enum opt_index {
  SCALE = 0,
  PERIOD = 1,
  STOP = 2,
  START = 3,
  LOG = 4,
  OFF = 5
};
char* opt_name[6] = {
  "SCALE",
  "PERIOD",
  "STOP",
  "START",
  "LOG",
  "OFF"
};

void getTimestamp(char *time_str) {
  time(&raw_time);
  current_time = localtime(&raw_time);
  sprintf(time_str, "%02d:%02d:%02d", current_time->tm_hour, current_time->tm_min, current_time->tm_sec);
}

float analogToTemp(int voltage) {
  float R = 1023.0/voltage-1.0;
  R = R0*R;
  float cel = 1.0/(log(R/R0)/B+1/298.15)-273.15;
  return cel;
}

void button_press() {
  exit(0);
}

void closeAndExit () {
  mraa_gpio_close(button);
  mraa_aio_close(tempSensor);
  char time[20];
  getTimestamp(time);
  // print timestamp 
  fprintf(stdout, "%s SHUTDOWN\n", time);
  if (logFd != -1) {
    dprintf(logFd, "%s SHUTDOWN\n", time);
    if (close(logFd) == -1) {
      fprintf(stderr, "I/O Error: Unable to close logfile, %s\n\r", strerror(errno));
      exit(1);
    }
  }
}

int main (int argc, char **argv) {

  long sample_period = 1000000;
  //by default F
  char scale = 'F';
  char *logFile = NULL;

  static struct option longOptions[] =
    {
      {"period", required_argument, 0, 'p'},
      {"scale", required_argument, 0, 's'},
      {"log", required_argument, 0, 'l'},
      {0, 0, 0, 0}
    };

  int status;
  int statusIndex = 0;
  while (1) {
    status = getopt_long(argc, argv, "", longOptions, &statusIndex);

    if(status ==-1)
    	break;

    switch(status){
    	case 0:
    	break;
    	case 'p':
    	//period option invoked
    	sample_period = (float) (sample_period * strtod(optarg, NULL));
    	break;
    	case 's':
    	//scale option invoked
    	if (strlen(optarg) == 1 && optarg[0] == 'C')
			scale = 'C';
      	else if (strlen(optarg) == 1 && optarg[0] == 'F')
			scale = 'F';
		else {
			fprintf(stderr, "Option Error: Bad --scale option selection, correct usage: --scale=[F,C]");
			exit(1);
      	}
    	break;
    	case 'l':
    	//log option invoked
    	logFile = optarg;
    	break;
    	case '?':
    	 fprintf(stderr, "Option Error: Unrecognized Option: Valid Options are: --period=[interval (default=1)] --scale=[F(default),C] --log=[log-filename]\n Proper usage: ./lab4b --period=3 --scale=C --log=logfile.txt\n");
      	 exit(1);
    	default:
    	 fprintf(stderr, "Option Error: Default case reached for option input with status %d\n", status);
      exit(1);
    }
}

//open log file
 if (logFile != NULL) {
    if ((logFd = open(logFile, O_WRONLY | O_CREAT | O_TRUNC, 00666)) == -1) {
      fprintf(stderr, "IO Error: Unable to open log file %s, %s", logFile, strerror(errno));
      exit(1);
    }
  }

//button and sensor set up at pins 60 and 1
  button = mraa_gpio_init(60);
  tempSensor = mraa_aio_init(1);

  atexit(closeAndExit);

  mraa_gpio_dir(button, MRAA_GPIO_IN);

  mraa_gpio_isr(button, MRAA_GPIO_EDGE_RISING, &button_press, NULL);

  int reading = 0;


 //set polling up
   struct pollfd polldata[1];
   polldata[0].fd = STDIN;
   polldata[0].events = POLLIN | POLLHUP | POLLERR;
   polldata[0].revents = 0;
   int pollStatus = 0;
   int bytesRead = 0;
   int cmdbufOffset = 0;

  //initalize buffers
   char buf[BUFSIZE];
   char cmdbuf[BUFSIZE];
   memset(buf, 0, BUFSIZE);
   memset(cmdbuf, 0, BUFSIZE);


   //begin sample

   while (sampleFlag) {
    if (!isStopped) {
      reading = mraa_aio_read(tempSensor);
      float temp = analogToTemp(reading);
      if (scale == 'F')
	    temp = (temp * 9 / 5) + 32;
      char time[20];
      getTimestamp(time);
      fprintf(stdout, "%s %.1f\n", time, temp);
      if (logFile != NULL) {
	      dprintf(logFd, "%s %.1f\n", time, temp);
      }
    }

    usleep(sample_period);

    //do polling and search for STOP and START
    if((pollStatus = poll(polldata, 2, 0)) < 0) {
      fprintf(stderr, "Polling Error: Failed poll attempt, %s\n\r", strerror(errno));
      exit(1);
    } else if (pollStatus < 1) {
      // no events 
      continue;
    } else {
      // process input
      if (polldata[0].revents & POLLIN) {
	if ((bytesRead = read (STDIN, buf, BUFSIZE)) < 0) {
	  fprintf(stderr, "IO Error: Unable to read from FD %d: %s\n\r", STDIN, strerror(errno));
	  exit(1);
	} else if (bytesRead > 0) {
	  memcpy(&cmdbuf[cmdbufOffset], &buf, bytesRead);
	  cmdbufOffset += bytesRead;
	  int i = 0;
	  for (i = 0; i < cmdbufOffset && cmdbufOffset != 0; i++) {
	    if (cmdbuf[i] == '\n') {
	      // log the command to file
	      if (logFile != NULL) {
		int a = 0;
		for (a = 0; a < i; a++) {
		  dprintf(logFd, "%c", cmdbuf[a]);
		}
		dprintf(logFd, "\n");
	      }

	      cmdbuf[i] = 0;
	      char *opt = NULL;
	      size_t optLen = 0;

	      // detect STOP and START and OFF input
	      if (strcmp(cmdbuf, opt_name[STOP]) == 0) {
				isStopped = 1;
	      } 
	      else if (strcmp(cmdbuf, opt_name[START]) == 0) {
				isStopped = 0;
	      } 
	      else if (strcmp(cmdbuf, opt_name[OFF]) == 0) {
			exit(0);
	      } 
	      else {
			ssize_t offset = 0;
		
		// search for = sign
		for (offset = 0; offset < i; offset++) {
		  if (cmdbuf[offset] == '=') {
		    cmdbuf[offset] = 0;
		    optLen = strlen(cmdbuf + offset + 1) + 1;
		    opt = (char*) malloc(sizeof(char) * optLen);
		    strcpy(opt, cmdbuf + offset + 1);
		    // opt now holds all of the option
		    if (strcmp(cmdbuf, opt_name[LOG]) == 0) {
		      if (logFile != NULL) {
					dprintf(logFd, "%s\n", opt);
		      }
		      break;
		    } 
		    else if (strcmp(cmdbuf, opt_name[SCALE]) == 0) {
		      // update scale
		      if (strlen(opt) != 1 && !(opt[0] == 'F' || opt[0] == 'C')) {
				break;
		      }
		      scale = opt[0];
		      break;
		    } 
		    else if (strcmp(cmdbuf, opt_name[PERIOD]) == 0) {
		      // update period
		      double newPeriod = strtod(opt, NULL);
		      if (newPeriod < 0) {
				break;
		      }
		      sample_period = (float) (newPeriod * 1e6);
		      break;
		    }
		  }
		}
	      }

	      // shift down the buffer
	      memcpy(cmdbuf, &cmdbuf[i+1], BUFSIZE - i - 1);
	      cmdbufOffset = cmdbufOffset - i - 1;
	      i = 0;
	    }
	  }
	}
      }
    }
  }
   exit(0);
}









