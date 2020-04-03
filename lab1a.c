//NAME: Christina Oliveira                                                                                                                                                          //EMAIL: christina.oliveira.m@gmail.com                                                                                                                                             //ID: 204803448 

#include <wait.h>
#include <stdlib.h>
#include <signal.h>
#include <getopt.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <poll.h>
//#include <wait.h>

//functions to help
static int shell_flag = 0;
struct termios savedAttr;
pid_t procid;

void writeBytes (int fd, const void *buf, size_t bytes) {
  if (write(fd, buf, bytes) < 0) {
    fprintf(stderr, "IO Error: Unable to write %s, %s", (char*)buf, strerror(errno));
    exit(1);
  } 
}

void signalHandler(int sigN) {
  if (sigN == SIGPIPE) {
    fprintf(stderr, "SIGPIPE");
    exit(1);
  }
}

void makePipe(int fd[2]) {
  if (pipe(fd) == -1) {
    fprintf(stderr, "Piping Error: Unable to create  pipe, %s\n", strerror(errno));
    exit(1);
  }
}

void closeCheck(int fd){
  if (close(fd) == -1) {
    fprintf(stderr, "IO Error: Unable to close file descriptor, %s\n", strerror(errno));
    exit(1);
  }
}

void dupCheck (int fd) {
  if (dup(fd) == -1) {
    fprintf(stderr, "IO Error: Unable to dup file descriptor, %s\n", strerror(errno));
    exit(1);
  }
}
void setTermAttr (struct termios* attr) {
  if (tcsetattr(0, TCSANOW, attr) < 0) {
    fprintf(stderr, "Set Attribute Error on fd 0: %s\n", strerror(errno));
    exit (2);
  }
}
void saveTermAttr (struct termios* attr) {
  if (tcgetattr(0, attr) < 0) {
    fprintf(stderr, "Get Attribute Error on fd 0: %s\n", strerror(errno));
    exit (2);
  }
}

void restoreSettings(){
  setTermAttr(&savedAttr);
  if (shell_flag) {
    int status, result;
    if ((result = waitpid(procid , &status, 0)) == -1) {
      fprintf(stderr, "WaitPid Error: Failure to wait for shell to close, %s\n", strerror(errno));
      exit(1);
    } else if (result == procid) {
      fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", (status & 0x007f), WEXITSTATUS(status));
    }
  }
}

//start of main routine
int main(int argc, char** argv){
  // pid_t procid;
  saveTermAttr(&savedAttr);
  atexit(restoreSettings);
  int status;
  char*  program = NULL;
  
  static struct option long_options [] = {
   //    {"shell", optional_argument, &shell_flag, 1},
    {"shell", optional_argument, NULL, 'a'},
    {0, 0, 0, 0}
  };


  //non-canonical
  struct termios attr; 
  saveTermAttr(&attr);
  attr.c_iflag = ISTRIP;
  attr.c_oflag = 0;
  attr.c_lflag = 0;
  setTermAttr(&attr);

  //option parsing
  while(1){
  int statusindex = 0;
  status = getopt_long(argc, argv, "", long_options, &statusindex);
 
  if(status == -1){
    break;
  } 
  switch(status){
  case 0:
    shell_flag = 1;
    break;
  case 'a':
    program = optarg;
    shell_flag = 1;
    break;
  case '?':
    fprintf(stderr, "Option Error: Invalid option.\nValid options are: --shell\nCorrect usage: ./lab1a --shell=helloWorld\n");
    exit(1);
    break;
  default:
    fprintf(stderr, "Default Error: Reached default switch case for options with status %d\n", status);
    abort();
  }
}

  //create and initialize buffer
  char buffer[256];
  char crlf[2] = {0x0D, 0x0A};
  char lf[1] = {0x0A};

  int pipefromparent[2];
  int pipetoparent[2];

  //execute basic functionality (no --shell option)
  if(!shell_flag){
    while(1){
    int readBytes;
    readBytes = read(0,buffer,256);
    //    char crlf[2] = {0x0D, 0x0A};
    // char lf[1] = {0x0A};
    //error has occured 
    if(readBytes < 0){
      fprintf(stderr, "IO error: File Descriptor 0 can't be read: %s\n",strerror(errno));
      exit(1);
    }
    //if bytes were read, check for lf and cr byte by byte
    else if(readBytes > 0){
      //C-d last byte?
      if (buffer[readBytes-1] == 0x04) {
	exit(0);
      } 
      else{
	for(int i = 0; i < readBytes; i++){
	  if((buffer[i] == 0x0D) || (buffer[i] == 0x0A)){
	    writeBytes(1, crlf, 2);
	  }
	  else{
	    writeBytes(1,&buffer[i],1);
	  }
	}
      }
    }
    }
  }

  //if shell flag is set
  else{
    //register err signals
    signal(SIGINT, signalHandler);
    signal(SIGPIPE, signalHandler);
   
    //open pipes
    // int pipefromparent[2];
    // int pipetoparent[2]; 
    makePipe(pipefromparent);
    makePipe(pipetoparent);

    procid = fork();
    if(procid == -1){
      fprintf(stderr, "Fork Error: %s\n", strerror(errno));
      exit(1);
    }
    //child process
    else if(procid == 0){
      //close approprate pipes, execute the shell
      // create_shell();
      char *arguments[] = {program, NULL};

      closeCheck(pipefromparent[1]);
      closeCheck(pipetoparent[0]);

      closeCheck(0);
      dupCheck(pipefromparent[0]);
      //close stdout                                                                                                                                                                     
      closeCheck(1);
      dupCheck(pipetoparent[1]);
      //close stderr                                                                                                                                                                     
      closeCheck(2);
      dupCheck(pipetoparent[1]);

      if (execvp(arguments[0], arguments) == -1) {
	fprintf(stderr, "Execution Err: Unable to  start %s, %s\n", program, strerror(errno));
	exit(1);
      }
    }
    //parent process
    else {
      //new thread
      // create_thread(procid);
      int pollStatus = 0;
      int readBytes = 0;

      closeCheck(pipefromparent[0]);
      closeCheck(pipetoparent[1]);

      //polling struct                                                                                                                                                                   
      struct pollfd polldata[2];

      polldata[0].fd = 0;
      polldata[0].events = POLLIN | POLLHUP | POLLERR;
      polldata[0].revents = 0;

      polldata[1].fd = pipetoparent[0];
      polldata[1].events = POLLIN | POLLHUP | POLLERR;
      polldata[1].revents = 0;

      while(1){

	if((pollStatus = poll(polldata, 2, 0)) < 0) {
	  fprintf(stderr, "Poll Error: Unable to poll, %s\n", strerror(errno));
	  exit(1);
	}
	else if (pollStatus < 1) {
	  continue;
	} else {
	  //event detected                          
	  if (polldata[0].revents & POLLIN) {
	    // read from std in                                                                                                                                                          
	    if ((readBytes = read (0, buffer, 256)) < 0) {
	      fprintf(stderr, "IO Error: Unable to read from fd %d: %s\n", 0, strerror(errno));
	      exit(1);
	    }
	    else if(readBytes > 0){
	      for (int i = 0; i < readBytes; i++) {
		if (buffer[i] == 0x0D || buffer[i] == 0x0A) {
		  writeBytes(1, crlf, 2);
		  writeBytes(pipefromparent[1], lf, 1);
		}
		else if(buffer[i] == 0x04) {
		    fprintf(stderr, "^D\n");
		    closeCheck(pipefromparent[1]);
		    while ((readBytes = read(pipetoparent[0], buffer, 256)) > 0) {
		      for (int j = 0; j < readBytes; j++) {
			if(buffer[j] == 0x0A){
			  writeBytes(1, crlf, 2);
			} else {
			  writeBytes(1, &buffer[j], 1);
			}
		      }
		    }
		    if (readBytes < 0) {
		      fprintf(stderr, "IO Error: Unable to read from fd %d: %s\n", 0, strerror(errno));
		      exit(1);
		    }
		    exit(0);
		  }
		  //upon finding ctrlC char kill the parent process                                                                                                                      
		  else if (buffer[i] == 0x03) {
		    fprintf(stderr, "^C\n");
		    kill(procid, SIGINT);
		  } else {
		    writeBytes(1, &buffer[i], 1);
		    writeBytes(pipefromparent[1], &buffer[i], 1);
		  }
		  }
	      }
	    }
	  }
	  //process output                                                                                                                                                               
	  if (polldata[1].revents & POLLIN) {
	    if ((readBytes = read(pipetoparent[0], buffer, 256)) < 0) {
	      fprintf(stderr, "IO Error: Unable to read from fd %d: %s\n", 0, strerror(errno));
	      exit(1);
	    }
	    else if (readBytes > 0) {
	      for (int k = 0; k < readBytes; k++) {
		if (buffer[k] == 0x0A) {
		  writeBytes(1, crlf, 2);
		} else {
		  writeBytes(1, &buffer[k], 1);
		}
	      }
	    }
	  }
	  if (polldata[0].revents & (POLLERR | POLLHUP) || polldata[1].revents & (POLLERR | POLLHUP))
	    exit(1);

	}
      }
    }

  exit (0);
}
