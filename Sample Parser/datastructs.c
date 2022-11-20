/***************************************************************
                         datastructs.c
                             
Contains useful data structures
***************************************************************/

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "main.h"


/*********************************
      Linked List Functions
*********************************/

/*==============================
    list_append
    Appends data to a linked list
    @param The linked list to append to
    @param The data to append
    @returns The created node
==============================*/

listNode* list_append(linkedList* list, void* data)
{
    // Allocate memory for our new node
    listNode* node = (listNode*)calloc(1, sizeof(listNode));
    if (node == NULL)
        terminate("Error: Unable to allocate memory for linked list node\n");
    node->data = data;
    
    // Assign the node to the list
    if (list->head == NULL)
        list->head = node;
    else
        list->tail->next = node;
    list->tail = node;

    // Increment the linked list size
    list->size++;
    
    // Return the created node
    return node;
}


/*==============================
    list_combine
    Attaches a list to the end of another list
    Assumes head is not null
    @param The destination list
    @param The list to attach
==============================*/

void list_combine(linkedList* dest, linkedList* list)
{
    // Make sure the lists are not null, and we're not feeding into ourselves
    if (list == NULL || dest == NULL || dest == list)
        return;
    
    // If the list has stuff in it, then attach to the tail. Otherwise, attach to the head
    if (dest->head != NULL)
    {
        dest->tail->next = list->head;
        dest->tail = list->tail;
    }
    else
    {
        dest->head = list->head;
        dest->tail = list->tail;
    }
    dest->size += list->size;
}


/*==============================
    list_remove
    Removes data from a linked list
    @param The linked list to append to
    @param The data to append
    @returns The removed node
==============================*/

listNode* list_remove(linkedList* list, void* data)
{
    listNode* lastnode = NULL, *thisnode;
    
    // Ensure the list is not null
    if (list == NULL)
        return NULL;
    
    // Iterate through the linked list
    for (thisnode = list->head; thisnode != NULL; thisnode = thisnode->next)
    {
        // If we found the data we want to remove
        if (thisnode->data == data)
        {
            // Arrange the list's pointers depending on where the data was
            if (thisnode == list->head)
            {
                list->head = thisnode->next;
                
                // If the only element in the list, we need to fix the tail too
                if (thisnode == list->tail) 
                    list->tail = NULL;
            }
            else if (thisnode == list->tail)
            {
                lastnode->next = NULL;
                list->tail = lastnode;
            }
            else
            {
                lastnode->next = thisnode->next;
            }
            
            // Decrement the list size, and then stop
            list->size--;
            return thisnode;
        }
        
        // Continue searching
        lastnode = thisnode;
    }
    return NULL;
}


/*==============================
    list_destroy
    Frees all the memory used by a linked list
    @param The linked list to destroy
==============================*/

void list_destroy(linkedList* list)
{
    listNode* curnode;
    
    // Make sure the list exists
    if (list == NULL)
        return;
    
    // Iterate through the linked list
    for (curnode = list->head; curnode != NULL;)
    {
        listNode* nextnode = curnode->next;
        
        // Free the node, then go to the next node
        free(curnode);
        curnode = nextnode;
    }
    
    // Set the linked list back to 0
    memset(list, 0, sizeof(linkedList));
}


/*==============================
    list_destroy_deep
    Frees all the memory used by a linked list, and its node's data
    @param The linked list to destroy
==============================*/

void list_destroy_deep(linkedList* list)
{
    listNode* curnode;
    
    // Make sure the list exists
    if (list == NULL)
        return;
    
    // Iterate through the linked list
    for (curnode = list->head; curnode != NULL;)
    {
        listNode* nextnode = curnode->next;
        
        // Free the data, then the node itself
        free(curnode->data);
        free(curnode);
        
        // Go to the next node
        curnode = nextnode;
    }
    
    // Set the linked list back to 0
    memset(list, 0, sizeof(linkedList));
}


/*==============================
    list_node_from_index
    Gets the n'th node from a list
    @param The linked list to search
    @param The index to find
    @returns The relevant node, or NULL
==============================*/

listNode* list_node_from_index(linkedList* list, int index)
{
    int i;
    listNode* curnode = list->head;
    for (i=0; i<index && curnode != NULL; i++)
        curnode = curnode->next;
    return curnode;
}


/*==============================
    list_index_from_data
    Finds the index of the node in the list, given the data
    @param The linked list to search
    @param The data to find
    @returns The index of the node, or -1
==============================*/

int list_index_from_data(linkedList* list, void* data)
{
    int index = 0;
    for (listNode* node = list->head; node != NULL; node = node->next, index++)
        if (node->data == data)
            return index;
    return -1;
}


/*==============================
    list_swapindex_withlist
    Replaces an element at an index with a list
    Assumes non null lists
    @param The destination list
    @param The index to change
    @param The list to attach
    @return The replaced node
==============================*/

listNode* list_swapindex_withlist(linkedList* dest, int index, linkedList* list)
{
    int cindex = 0;
    listNode* curnode = NULL, *previous = NULL;
    
    // Stop if the lists don't exist, the index is invalid, or the destination is empty
    if (dest == NULL || list == NULL || index < 0 || dest->size == 0 || list->size == 0)
        return NULL;
        
    // If we want the first element, then replace the head
    if (index == 0)
    {
        curnode = dest->head;
        list->tail->next = dest->head->next;
        dest->head = list->head;
        dest->size += list->size-1;
        return curnode;
    }
        
    // Go to the n'th element
    for (curnode = dest->head; curnode != NULL; curnode = curnode->next)
    {
        if (cindex == index)
            break;
        cindex++;
        previous = curnode;
    }
    
    // Stop if the element wasn't found
    if (curnode == NULL)
        return NULL;
        
    // Replace the pointers
    previous->next = list->head;
    list->tail->next = curnode->next;
    dest->size += list->size-1;
    
    // Correct the head and tail if needed
    if (dest->head == curnode)
        dest->head = list->head;
    if (dest->tail == curnode)
        dest->tail = list->tail;
    
    // Return the replaced node
    return curnode;
}


/*==============================
    list_hasvalue
    Returns whether a value is in a list
    @param The list to check
    @param The data to check for
    @return Whether the value is in the list
==============================*/

bool list_hasvalue(linkedList* list, void* data)
{
    for (listNode* node = list->head; node != NULL; node = node->next)
        if (node->data == data)
            return TRUE;
    return FALSE;
}


/*********************************
       Dictionary Functions
*********************************/

/*==============================
    dict_append
    Appends data to a dictionary
    @param The dictionary to append to
    @param The key of the data
    @param The data to append
    @returns The created node
==============================*/

dictNode* dict_append(Dictionary* dict, int key, void* value)
{
    // Allocate memory for our new node
    dictNode* node = (dictNode*)calloc(1, sizeof(dictNode));
    if (node == NULL)
        terminate("Error: Unable to allocate memory for dictionary node\n");
    node->key = key;
    node->value = value;
    
    // Assign the node to the dictionary
    if (dict->head == NULL)
        dict->head = node;
    else
        dict->tail->next = node;
    dict->tail = node;

    // Increment the dictionary size
    dict->size++;
    
    // Return the created node
    return node;
}


/*==============================
    dict_getkey
    Checks if a dictionary has a key
    @param The dictionary to search
    @param The key to search
    @returns The node with the value, or NULL
==============================*/

dictNode* dict_getkey(Dictionary* dict, int key)
{
    dictNode* curnode;
    
    // Make sure we have a valid dictionary
    if (dict == NULL)
        return NULL;

    // Search through the dictionary
    for (curnode = dict->head; curnode != NULL; curnode = curnode->next)
        if (curnode->key == key)
            return curnode;
    
    // Didn't find the key, return NULL
    return NULL;
}


/*==============================
    dict_destroy
    Frees all the memory used by a dictionary
    @param The dictionary to destroy
==============================*/

void dict_destroy(Dictionary* dict)
{
    dictNode* curnode = dict->head;
    dictNode* nextnode = dict->head;
    
    // Iterate through the dictionary
    while (curnode != NULL)
    {
        nextnode = curnode->next;
        
        // Free the node, and then go to the next node
        free(curnode);
        curnode = nextnode;
    }
    memset(dict, 0, sizeof(Dictionary));
}


/*==============================
    dict_destroy_deep
    Frees all the memory used by a dictionary, and its node's data
    @param The dictionary to destroy
==============================*/

void dict_destroy_deep(Dictionary* dict)
{
    dictNode* curnode = dict->head;
    dictNode* nextnode = dict->head;
    
    // Iterate through the dictionary
    while (curnode != NULL)
    {
        nextnode = curnode->next;
        
        // Free the node data, and then the node itself
        free(curnode->value);
        free(curnode);
            
        // Go to the next node
        curnode = nextnode;
    }
    memset(dict, 0, sizeof(Dictionary));
}


/*********************************
       Hashtable Functions
*********************************/

/*==============================
    htable_append
    Adds an element to a hashtable
    @param The destination list
    @param The list to attach
==============================*/

dictNode* htable_append(hashTable* htable, int key, void* value)
{
    htable->size++;
    return dict_append(&htable->table[key%HASHTABLE_SIZE], key, value);
}


/*==============================
    htable_getkey
    Checks if a hashtable has a key
    @param The hashtable to search
    @param The key to search
    @returns The node with the value, or NULL
==============================*/

dictNode* htable_getkey(hashTable* htable, int key)
{
    return dict_getkey(&htable->table[key%HASHTABLE_SIZE], key);
}


/*==============================
    htable_destroy
    Frees all the memory used by a hashtable
    @param The hashtable to destroy
==============================*/

void htable_destroy(hashTable* htable)
{
    int i;
    for (i=0; i<HASHTABLE_SIZE; i++)
        dict_destroy(&htable->table[i]);
    memset(htable, 0, sizeof(hashTable));
}


/*==============================
    htable_destroy_deep
    Frees all the memory used by a hashtable, and its node's data
    @param The hashtable to destroy
==============================*/

void htable_destroy_deep(hashTable* htable)
{
    int i;
    for (i=0; i<HASHTABLE_SIZE; i++)
        dict_destroy_deep(&htable->table[i]);
    memset(htable, 0, sizeof(hashTable));
}


/*********************************
         Other Functions
*********************************/

/*==============================
    float_to_s10p5
    Converts a float to fixed point in s10.5 format
    @param The float to convert
    @returns The converted fixed point number
==============================*/

inline short float_to_s10p5(double input)
{
    return (short)(round(input) * (1 << 5));
}


/*==============================
    vector_scale
    Scales a vector by an amount
    @param The vector to scale
    @param The scale amount
    @returns The scaled vector
==============================*/

Vector3D vector_scale(Vector3D vec, float scale)
{
    vec.x *= scale;
    vec.y *= scale;
    vec.z *= scale;
    return vec;
}


/*==============================
    nearest_pow2
    Returns the nearest power of two for a number
    @param The number to round
    @returns The rounded value
==============================*/

inline int nearest_pow2(int n)
{
    return (int)ceil(log(n)/log(2));
}
