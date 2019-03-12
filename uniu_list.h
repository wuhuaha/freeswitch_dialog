#ifndef __UNIU_LIST_H__
#define __UNIU_LIST_H__
#include<stdio.h>
#ifndef SAFE_FREE
#define SAFE_FREE(ptr) \
{ \
    if (NULL != ptr) { \
        free(ptr); \
        ptr = NULL; \
    } \
}
#endif
typedef unsigned short ushort_t;
typedef unsigned char uchar_t;
typedef unsigned int uint_t;

#define ___ALLOCATION_ERROR___                             \
    printf("alloce error");        \
exit(1);

/* {{{ link list interface define::start*/
struct link_node {
    void *value;
    struct link_node *prev;
    struct link_node *next;
};
typedef struct link_node link_node_entry;
typedef link_node_entry * link_node_t;

/*
 * link list adt
 */
typedef struct {
    link_node_t head;
    link_node_t tail;
    uint_t size;
} link_entry;

typedef link_entry * link_t;

//create a new link list
link_t new_link_list( void );

//free the specified link list
void free_link_list( link_t );

//free the given link list and  values in it
void free_link_list_and_value( link_t link );

//return the size of the current link list.
//uint_t link_list_size( link_t );
#define link_list_size( link ) link->size

//check the given link is empty or not.
//int link_list_empty( link_t );
#define link_list_empty( link ) (link->size == 0)

//clear all the nodes in the link list( except the head and the tail ).
link_t link_list_clear( link_t link );

//clear all nodes and value in the link list
link_t link_list_clear_free( link_t link );

//add a new node to the link list.(append from the tail)
void link_list_add( link_t, void * );

//add a new node before the specified node
void link_list_insert_before( link_t, uint_t, void * );

//get the node in the current index.
void *link_list_get( link_t, uint_t );

//modify the node in the current index.
void *link_list_set( link_t, uint_t, void * );

//remove the specified link node
void *link_list_remove( link_t, uint_t );

//remove the given node
void *link_list_remove_node( link_t, link_node_t );

//remove the node from the frist.
void *link_list_remove_first( link_t );

//remove the last node from the link list
void *link_list_remove_last( link_t );

//append a node from the end.
void link_list_add_last( link_t, void * );

//add a node at the begining of the link list.
void link_list_add_first( link_t, void * );
/* }}} link list interface define::end*/
#endif