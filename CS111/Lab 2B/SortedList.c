//NAME: Darren Lu
//EMAIL: darrenlu3@ucla.edu
//ID: 205394473

#include "SortedList.h"
#include <string.h>
#include <stdio.h>
#include <sched.h>

/**
 * SortedList_insert ... insert an element into a sorted list
 *
 *The specified element will be inserted in to
 *the specified list, which will be kept sorted
 *in ascending order based on associated keys
 *
 * @param SortedList_t *list ... header for the list
 * @param SortedListElement_t *element ... element to be added to the list
 */
void SortedList_insert(SortedList_t *list, SortedListElement_t *element){
  //check if list and element provided are valid
  if (list == NULL || element == NULL){
    fprintf(stderr, "List or element provided were NULL!");
    return;
  }
  //find the element to insert before
  SortedListElement_t* b4_node = list->next;
  //if the list is empty (only has a head node) link the element circularly to it
  if(opt_yield & INSERT_YIELD) sched_yield();
  if (b4_node == NULL || b4_node->key == NULL){
    list->next = element;
    list->prev = element;
    element->next = list;
    element->prev = list;
  }
  else{ //otherwise find the element to insert before
    while (b4_node->key != NULL && strcmp(element->key,b4_node->key) > 0){
      b4_node = b4_node->next;
    }
    element->next = b4_node;
    element->prev = b4_node->prev;
    b4_node->prev->next = element;
    b4_node->prev = element;
  }
}

/**
 * SortedList_delete ... remove an element from a sorted list
 *
 *The specified element will be removed from whatever
 *list it is currently in.
 *
 *Before doing the deletion, we check to make sure that
 *next->prev and prev->next both point to this node
 *
 * @param SortedListElement_t *element ... element to be removed
 *
 * @return 0: element deleted successfully, 1: corrtuped prev/next pointers
 *
 */
int SortedList_delete( SortedListElement_t *element){
  if (element == NULL){
    fprintf(stderr, "Null element provided to sortedlist_delete\n");
    return 1;
  }
  if (element->key == NULL){
    fprintf(stderr, "Tried to delete linked list header!\n");
    return 1;
  }
  if (element->next->prev != element  || element->prev->next != element) return 1;
  if (opt_yield & DELETE_YIELD) sched_yield();
  element->prev->next = element->next;
  element->next->prev = element->prev;
  return 0;
}

/**
 * SortedList_lookup ... search sorted list for a key
 *
 *The specified list will be searched for an
 *element with the specified key.
 *
 * @param SortedList_t *list ... header for the list
 * @param const char * key ... the desired key
 *
 * @return pointer to matching element, or NULL if none is found
 */
SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key){
  if (list == NULL || key == NULL || list->next == NULL) return NULL;
  if (opt_yield & LOOKUP_YIELD) sched_yield();
  SortedListElement_t* current = list->next;
  while (current->key != NULL && strcmp(current->key,key) < 0){
    current = current->next;
  }
  if (current->key == NULL) return NULL;
  return current;
}

/**
 * SortedList_length ... count elements in a sorted list
 *While enumeratign list, it checks all prev/next pointers
 *
 * @param SortedList_t *list ... header for the list
 *
 * @return int number of elements in list (excluding head)
 *   -1 if the list is corrupted
 */
int SortedList_length(SortedList_t *list){
  if (list == NULL) return -1;
  if (opt_yield & LOOKUP_YIELD) sched_yield();
  SortedListElement_t* current = list->next;
  int counter = 0;
  while (current->key != NULL){
    if (current->next->prev != current || current->prev->next != current) return -1;
    counter++;
    current = current->next;
  }
  return counter;
}
