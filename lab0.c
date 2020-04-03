//NAME: Christina Oliveira                                                                                                                                                          //EMAIL: christina.oliveira.m@gmail.com                                                                                                                                             //ID: 204803448 

#include <stdlib.h>
#include <signal.h>
#include <getopt.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

static int segfault_flag = 0;
static int catch_flag = 0;

void throwseg();
void sighandler();

int main(int argc, char** argv){

  //process all arguments and store the results
  char* input = NULL;
  char* output= NULL;
  int status;
  while(1){
  //init options
  static struct option long_options [] = {
    {"input", required_argument, NULL, 'a'},
    {"output", required_argument, NULL, 'b'},
    {"segfault", no_argument, &segfault_flag, 1},
    {"catch", no_argument, &catch_flag, 1},
    {0, 0, 0, 0}
  };
  int statusindex = 0;
  status = getopt_long(argc, argv, "", long_options, &statusindex);
 
  if(status == -1){
    break;
  }
  switch(status){
  case 0:
    break;
  case 'a':
    input = optarg;
    break;
  case 'b':
    output = optarg;
    break;
  case '?':
    fprintf(stderr, "Option Error: Invalid option.\nValid options are: --input --output --segfault --catch\nCorrect usage: ./lab0 --segfault --catch --input input.txt --output output.txt\n");
    exit(1);
    break;
  default:
    fprintf(stderr, "Default Error: Reached default switch case for options with status %d\n", status);
    abort();
  }
  }

  //file redirection
  if(input != NULL){
    int inputsource = 0;
    if ((inputsource = open(input, O_RDONLY)) < 0) {
      fprintf(stderr, "IO Error: Unable to open %s for --input: %s\n", input, strerror(errno));
      exit(2);
    }

    if (close(0) < 0) {
      fprintf(stderr, "IO Error: Error closing STDIN for --input redirection: %s\n", strerror(errno));
      exit(5);
    }
    
    if (dup(inputsource) < 0) {
      fprintf(stderr, "IO Error: Error duplicating %s to FD 0 for --input redirection: %s\n", input, strerror(errno));
      exit(5);
    }
  }

  // file redirection of output source
  if (output != NULL) {
    int outputsource = 1;
    if ((outputsource = open(output, O_WRONLY | O_CREAT, 00666)) < 0) {
      fprintf(stderr, "IO Error: Unable to open %s for --output: %s\n", output, strerror(errno));
      exit(3);
    }

    if (close(1) < 0) {
      fprintf(stderr, "IO Error: Error closing STDOUT for --output redirection: %s\n", strerror(errno));
      exit(5);
    }

    if (dup(outputsource) < 0) {
      fprintf(stderr, "IO Error: Error duplicating %s to FD 1 for --output redirection: %s\n", input, strerror(errno));
      exit(5);
    }
  }


  //register the signal handler
  if(catch_flag){
    //report catch
    signal(SIGSEGV, sighandler);
  }

  //cause the seg fault
  if(segfault_flag){
    throwseg();
  }

  //if no ssegfault was caused, copt stdin to stdout
  char currchar;
  int stat = 0;
  while ((stat = read(0, &currchar, 1)) == 1) {
    if (stat < 0) {
      fprintf(stderr, "IO Error: Error reading file %s: %s\n", input, strerror(errno));
      exit(5);
    }
    if (write(1, &currchar, 1) < 0) {
      fprintf(stderr, "IO Error: Error writing to file %s: %s\n", output, strerror(errno));
    }
  }

  // close streams
  if (close(0) < 0) {
    fprintf(stderr, "IO Error: Unable to close %s as input file: %s\n", input, strerror(errno));
    exit(5);
  }

  if (close(1) < 0) {
    fprintf(stderr, "IO Error: Unable to close %s as output file: %s\n", output, strerror(errno));
    exit(5);
  }
  //otherwise, sucess
  exit(0);
}

  //aux functions
  void throwseg() {
    char* invalid = NULL;
    *invalid = 'c';
  }

  void sighandler () {
    fprintf(stderr, "SIGSEGV Error: Successful --catch option\n");
    exit(4);
  }
