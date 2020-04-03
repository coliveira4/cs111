//NAME: Christina Oliveira
//EMAIL: christina.oliveira.m@gmail.com
//ID: 204803448
//lab2_list.c ... a C program that implements the (below) specified command 
//line options and produces the (below) specified output statistics.

#include "SortedList.h"
#include <getopt.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#define KEYLEN 15
#define NUMLETTERS 26

long long totalIterations = 0;
int numIterations = 1;
int numThreads = 1;
int opt_yield = 0;
int lock = 0;
int listLength = 0;
int yieldModeLength = 0;
char syncArg = '\0';
char yieldMode[4] = "\0\0\0\0";
pthread_mutex_t mutex;
//initialize a Sortedlist
SortedList_t *list;
SortedListElement_t *initElems;

void initElements() {
  srand(time(0));
  for (int i = 0; i < totalIterations; i++) {
    int length = (rand() % KEYLEN) + 5;
    int letter = (rand() % NUMLETTERS);
    char *randomKey = malloc(sizeof(char) * (length + 1));
    for (int a = 0; a < length; a++) {
      randomKey[a] = 'a' + letter;
      letter = rand() % NUMLETTERS;
    }
    randomKey[length] = '\0';
    initElems[i].key = randomKey;
  }
}

void handleYieldModes(char* options) {
  for (int i = 0; options[i] != '\0'; i++) {
    if (options[i] == 'i') {
      opt_yield |= INSERT_YIELD;
      yieldMode[yieldModeLength++] = 'i';
    } else if (options[i] == 'd') {
      opt_yield |= DELETE_YIELD;
      yieldMode[yieldModeLength++] = 'd';
    } else if (options[i] == 'l') {
      opt_yield |= LOOKUP_YIELD;
      yieldMode[yieldModeLength++] = 'l';
    } else {
      fprintf(stderr, "Option Error: Invalid argument for option --yield=[idl]");
      exit(1);
    }
  }
}

void* manipulateThreads (void *threadId) {
  for (int i = (*(int*)threadId); i < totalIterations; i += numThreads) {
    switch(syncArg) {
      case 'm':
		pthread_mutex_lock(&mutex);
		SortedList_insert(list, &initElems[i]); 
		pthread_mutex_unlock(&mutex);
		break;
      case 's':
		while(__sync_lock_test_and_set(&lock, 1));
		SortedList_insert(list, &initElems[i]);
		__sync_lock_release(&lock);
		break;
      default:
		SortedList_insert(list, &initElems[i]);
		break;
    	}
  }

	switch (syncArg) {
  	case 'm':
    	pthread_mutex_lock(&mutex);
    	listLength = SortedList_length(list);
    	pthread_mutex_unlock(&mutex);
    break;
  case 's':
    while(__sync_lock_test_and_set(&lock, 1));
    listLength = SortedList_length(list);
    __sync_lock_release(&lock);
    break;
  default:
    listLength = SortedList_length(list);
    break;
  }
  
	//  if (listLength < 0) {
	// fprintf(stderr, "Corruption Detected: Post-element insertion, call to SortedList_length failed with value %d\n", listLength);
	//exit(2);
	//}
  SortedListElement_t *temp;

  for (int i = (*(int*)threadId); i < totalIterations; i += numThreads) {
    switch(syncArg)
      {
      case 'm':
	pthread_mutex_lock(&mutex);
	temp = SortedList_lookup(list, initElems[i].key);
	//if (temp == NULL) {
	//fprintf(stderr, "Corrupt List: Unable to find key that was inserted\n");
	// exit(2);
	//}
	if (SortedList_delete(temp)) {
	  //	  fprintf(stderr, "Corrupted List: deletion failed\n");
	  //exit(2);
	}
	pthread_mutex_unlock(&mutex);
	break;
      case 's':
	while(__sync_lock_test_and_set(&lock, 1));
	temp = SortedList_lookup(list, initElems[i].key);
	//if (temp == NULL) {
	//  fprintf(stderr, "Corruptted List: Can't to find key that was inserted\n");
        //  exit(2);
	//}
	if (SortedList_delete(temp)) {
	  // fprintf(stderr, "Corrupted List: deletion failed\n");
	  // exit(2);
        }
	__sync_lock_release(&lock);
	break;
      default:
	temp = SortedList_lookup(list, initElems[i].key);
	//if (temp == NULL) {
	// fprintf(stderr, "Corrupted List: Unable to find key that was inserted\n");
	// exit(2);
	//}
	if (SortedList_delete(temp)) {
          //fprintf(stderr, "Corrupted List: delete failed\n");
          //exit(2);
        }
	break;
      }
  }

  switch (syncArg) {
  case 'm':
    pthread_mutex_lock(&mutex);
    listLength = SortedList_length(list);
    pthread_mutex_unlock(&mutex);
    break;
  case 's':
    while(__sync_lock_test_and_set(&lock, 1));
    listLength = SortedList_length(list);
    __sync_lock_release(&lock);
    break;
  default:
    listLength = SortedList_length(list);
    break;
  }
  // if (listLength < 0) {
  //  fprintf(stderr, "Corrupt List: Post-element deletion, SortedList_length failed with value %d\n", listLength);
  //  exit(2);
  // }
  return NULL;
}

void sighandler() {
  fprintf(stderr, "SIGSEGV Error: Segmentation fault detected\n");
  exit(2);
}

int main (int argc, char** argv) {
  signal(SIGSEGV, sighandler);
  static struct option longOptions[] = 
    {
      {"threads", required_argument, 0, 't'},
      {"iterations", required_argument, 0, 'i'},
      {"yield", required_argument, 0, 'y'},
      {"sync", required_argument, 0, 's'},
      {0, 0, 0, 0}
    };
  int status;
  int statusIndex = 0;
  while (1) {
    status = getopt_long(argc, argv, "", longOptions, &statusIndex);
    if (status == -1)
      break;
    switch (status) {
    case 0:
      break;
    case 't':
      numThreads = atoi(optarg);
      break;
    case 'i':
      numIterations = atoi(optarg);
      break;
    case 'y':
      handleYieldModes(optarg);
      break;
    case 's':
      if (strlen(optarg) == 1 && optarg[0] == 'm') {
		syncArg = 'm';
      } 
      else if (strlen(optarg) == 1 && optarg[0] == 's') {
		syncArg = 's';
      } 
      else {
	fprintf(stderr, "Option Error: Invalid character for option --sync, Valid options are m,s,c\n");
	exit(1);
      }
      break;
    case '?':
      // unrecognized option
      fprintf(stderr, "Unrecognized Option: Valid Options: --threads=[thread number] --iterations=[num of iterations] --yield=idl --sync[msc]\n Proper usage: ./lab2_add --threads=10 --iterations=10 --yield=idl --sync=m\n");
      exit(1);
    default:
      fprintf(stderr, "Option Switch Error: Reached default case for option input with status %d\n", status);
      exit(1);
    }
  }

  if (syncArg == 'm') {
    if (pthread_mutex_init(&mutex, NULL)) {
      fprintf(stderr, "Mutex Initialization Error: Failed to initialize mutex, %s\n", strerror(errno));
      exit(2);
    }
  }
  totalIterations = numThreads * numIterations;

  list = malloc(sizeof(SortedList_t));
  if (list == NULL) {
    fprintf(stderr, "List Allocation Error: Unable to allocate initial list\n");
    exit(2);
  }
  list->key = NULL;
  list->next = list;
  list->prev = list;
  initElems = NULL;
  initElems = malloc(sizeof(SortedListElement_t) * totalIterations);

  if (initElems == NULL) {
    fprintf(stderr, "List Allocation Error: Unable to allocate space for initialized elements\n");
    exit(2);
  }

  initElements();

  struct timespec start;
  struct timespec end;
  if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start) < 0) {
    fprintf(stderr, "Clock Error: %s\n", strerror(errno));
    exit(2);
  }
  
  pthread_t *threads = NULL;
  threads = malloc(numThreads * sizeof(pthread_t));
  if (threads == NULL) {
    fprintf(stderr, "Thread Allocation Error: Unable to allocate space for threads\n");
    exit(2);
  }

  int *threadIds = NULL;
  threadIds = malloc(numThreads * sizeof(int));
  if (threadIds == NULL) {
    fprintf(stderr, "Thread Allocation Error: Unable to allocate space for thread IDs\n");
    exit(2);
  }

  for (int i = 0; i < numThreads; i++) {
    threadIds[i] = i;
    if(pthread_create(&threads[i], NULL, manipulateThreads, &threadIds[i])) {
      fprintf(stderr, "Thread Creation Error: Unable to create thread, %s\n", strerror(errno));
      exit(2);
    }
  }

  for (int i = 0; i < numThreads; i++) {
    if (pthread_join(threads[i], NULL)) {
      fprintf(stderr, "Thread Join Error: Unable to join thread, %s\n", strerror(errno));
      exit(2);
    }
  }

  //if (SortedList_length(list) != 0) {
  // fprintf(stderr, "Corruption Error: List length is non-zero after completion\n");
  //  exit(2);
  //}

  if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end) < 0) {
    fprintf(stderr, "Clock Error: %s\n", strerror(errno));
    exit(2);
  }

  long totalTime = (1000000000 * (end.tv_sec - start.tv_sec)) + (end.tv_nsec - start.tv_nsec);
  int numOperations = totalIterations * 3;

  if (strlen(yieldMode) == 0) {
    if (syncArg == '\0') {
      fprintf(stdout, "list-none-none,%d,%d,1,%d,%ld,%ld\n", numThreads, numIterations, numOperations, totalTime, totalTime / numOperations);
    } else if (syncArg == 's') {
      fprintf(stdout, "list-none-s,%d,%d,1,%d,%ld,%ld\n", numThreads, numIterations, numOperations, totalTime, totalTime / numOperations);
    } else if (syncArg == 'm') {
      fprintf(stdout, "list-none-m,%d,%d,1,%d,%ld,%ld\n", numThreads, numIterations, numOperations, totalTime, totalTime / numOperations);
    }
  } else {
    if (syncArg == '\0') {
      fprintf(stdout, "list-%s-none,%d,%d,1,%d,%ld,%ld\n", yieldMode, numThreads, numIterations, numOperations, totalTime, totalTime / numOperations);
    } else if (syncArg == 's') {
      fprintf(stdout, "list-%s-s,%d,%d,1,%d,%ld,%ld\n", yieldMode, numThreads, numIterations, numOperations, totalTime, totalTime / numOperations);
    } else if (syncArg == 'm') {
      fprintf(stdout, "list-%s-m,%d,%d,1,%d,%ld,%ld\n", yieldMode, numThreads, numIterations, numOperations, totalTime, totalTime / numOperations);
    }
  }
  free(threads);
  free(threadIds);
  free(initElems);
  exit(0);
}
