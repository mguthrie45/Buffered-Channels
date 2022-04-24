#include "linked_list.h"
#include <stdio.h>
#include <stdbool.h>

bool check_nodes(list_t* list){
    int count = (int)(list_count(list));
    int aCount = 0;

    list_node_t *cur = list -> head;
    list_node_t *prev = NULL;
    
    if (list -> head == NULL && count > 0) {
        printf("list head was lost\n");
        return false;
    }

    while (cur != NULL){
        if (cur -> prev != prev){
            return false;
        }
        prev = cur;
        cur = cur -> next;
        aCount++;
    }
    if (aCount != count){
        return false;
    }
    return true;
}

// Creates and returns a new list
list_t* list_create()
{
    /* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */
    list_t *lst = malloc(sizeof(list_t));
    lst -> count = 0;
    lst -> head = NULL;
    return lst;
}

// Destroys a list
void list_destroy(list_t* list)
{
    /* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */
    list_node_t *cur = list -> head;
    while (cur != NULL) {
        list_remove(list, list -> head);
        cur = cur -> next;
    }
    free(list);
}

// Returns beginning of the list
list_node_t* list_begin(list_t* list)
{
    /* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */
    if (list -> count >= 1)
        return list -> head;
    return NULL;
}

// Returns next element in the list
list_node_t* list_next(list_node_t* node)
{
    /* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */
    return node -> next;
}

// Returns data in the given list node
void* list_data(list_node_t* node)
{
    /* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */
    return node -> data;
}

// Returns the number of elements in the list
size_t list_count(list_t* list)
{
    /* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */
    return list -> count;
}

// Finds the first node in the list with the given data
// Returns NULL if data could not be found
list_node_t* list_find(list_t* list, void* data)
{
    /* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */
    //printf("list find\n");
    if (list -> count <= 0)
        return NULL;
       
    list_node_t *cur = list -> head;
    while (cur != NULL) {
        if (cur -> data == data) {
            //printf("found\n");
            return cur;
        }
        cur = cur -> next;
    }
    //printf("not found\n");
    return NULL;
}

// Inserts a new node in the list with the given data
void list_insert(list_t* list, void* data)
{
    /* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */
    list_node_t *new_node = malloc(sizeof(list_node_t));
    new_node -> prev = NULL;
    //if (list -> head == NULL) {
    	//printf("ins head NULL before\n");
    //}
    //printf("node stuff\n");
    new_node -> data = data;
    new_node -> next = list -> head;
    new_node -> prev = NULL;

    if (new_node -> next != NULL){
        //printf("head is not null\n");
        list -> head -> prev = new_node;
    }
    list -> head = new_node;
    
    list -> count += 1;
    //printf("increment count %ld\n", list -> count);
    //printf("--------------LIST CHECK: %i-------------\n", (int)check_nodes(list));
    //if (list -> head == NULL) {
    	//printf("ins NOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO\n");
    //}
}

// Removes a node from the list and frees the node resources
void list_remove(list_t* list, list_node_t* node)
{
    /* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */
    //printf("Remove Node\n");
    if (node == NULL)
        return;
    //list_node_t *prv = node -> prev;
    //list_node_t *nxt = node -> next;
    
    //if (node -> next == NULL)
    	//printf("nxt is NULL\n");
    
    if (node -> prev == NULL) {
    	//printf("move head\n");
        list -> head = node -> next;
    }
    else {
        node -> prev -> next = node -> next;
    }
    if (node -> next != NULL){
        node -> next -> prev = node -> prev;
    }
    free(node);
    list -> count -= 1;
    //printf("decrement count %ld\n", list -> count);
    printf("--------------LIST CHECK: %i-------------\n", (int)check_nodes(list));
    //if (list -> head == NULL) {
    	//printf("rem NOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO\n");
    //}
    //printf("count %ld\n", list -> count);
}

// Executes a function for each element in the list
void list_foreach(list_t* list, void (*func)(void* data))
{
    /* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */
    list_node_t *cur = list -> head;
    //if (cur == NULL)
        //printf("Shouldn't be NULL\n");
    while (cur != NULL) {
        //printf("iterated %p\n", cur -> data);
        func(list_data(cur));
        //printf("here\n");
        cur = cur -> next;
    }
}

