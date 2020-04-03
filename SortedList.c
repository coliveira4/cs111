//NAME: Christina Oliveira
//EMAIL: christina.oliveira.m@gmail.com
//ID: 204803448
//SortedList.c ... a C module that implements insert, delete, lookup, and length methods for 
//a sorted doubly linked list (described in the provided header file, including correct placement of yield calls).

#include "SortedList.h"
#include <stdlib.h>
#include <pthread.h>
#include <string.h>


//Deletion for SortedList
int SortedList_delete(SortedListElement_t *element) {
	//error checks
  if (element == NULL || element->key == NULL)
    return 1;
  if (element->prev->next != element || element->next->prev != element)
    return 1;
//delete element
  if (opt_yield & DELETE_YIELD)
    sched_yield();
  element->prev->next = element->next;
  element->next->prev = element->prev;
  return 0;
}

//Lookup for SortedList
SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key) {
  if (list == NULL || list->key != NULL)
    return NULL; 
  
  SortedListElement_t *currElem = list->next;
  while (currElem != list) {
    // element found
    if (!strcmp(key, currElem->key))
      return currElem;
    if (opt_yield & LOOKUP_YIELD)
      sched_yield();
    currElem = currElem->next;
  }
//error finding element
  return NULL; 
}

//length for SortedList
int SortedList_length(SortedList_t *list) {
  	int size = 0;
  if (list == NULL)
    return -1;
  SortedListElement_t *currElem = list->next;
  while (currElem != list) {
    if (currElem->prev->next != currElem || currElem->next->prev != currElem)
      return -2;
    size++;
    if (opt_yield & LOOKUP_YIELD)
      sched_yield();
    currElem = currElem->next;
  }
  return size;
}

//insertion for Sortedlist
void SortedList_insert(SortedList_t *list, SortedListElement_t *element) {
   if (list == NULL || element == NULL)
      return;
	SortedListElement_t* currElem = list->next;
	while (currElem != list) {
    	if (strcmp(element->key, currElem->key) <= 0)
      		break;
    currElem = currElem->next;
  }
  if(opt_yield & INSERT_YIELD){
  	 sched_yield();
  }
  element->next = currElem;
  element->prev = currElem->prev;
  currElem->prev->next = element;
  currElem->prev = element;
  return;
}


