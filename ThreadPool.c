#define _GNU_SOURCE
#define __USE_GNU
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include "ThreadPool.h"

/* Initialise the thread pool. */
bool initialise_thread_pool(struct thread_pool *thread_pool)
{
    thread_pool->head = malloc(sizeof(struct node));
    thread_pool->tail = malloc(sizeof(struct node));
    if (thread_pool->head == NULL || thread_pool->tail == NULL)
        return false;

    thread_pool->head->prev = NULL;
    thread_pool->head->next = thread_pool->tail;
    thread_pool->tail->prev = thread_pool->head;
    thread_pool->tail->next = NULL;

    return true;
}

/* Create a new node and set the thread field to the passed in thread. */
struct node *new_node(pthread_t thread) 
{
    struct node *node = malloc(sizeof(struct node));
    if (node == NULL)
        return NULL;

    node->thread = thread;
    return node;
}

/* Add a new node to the back of the thread pool. */
bool add_to_thread_pool(struct thread_pool *thread_pool, pthread_t thread)
{
    struct node *node = new_node(thread);
    if (node == NULL)
        return false;
    
    thread_pool->tail->prev->next = node;
    node->prev = thread_pool->tail->prev;
    node->next = thread_pool->tail;
    thread_pool->tail->prev = node;
    return true;
}

/* Remove a node from the thread pool. */
void remove_node(struct node *node) 
{
    node->prev->next = node->next;
    node->next->prev = node->prev;
}

/* Iterate through all threads in the thread pool and join them back to main. */
void join_threads(struct thread_pool *thread_pool)
{
    struct node *prev = NULL;
    struct node *curr = thread_pool->head;
    while (curr != NULL) {
        prev = curr;
        curr = curr->next;
        if (prev != thread_pool->head && prev != thread_pool->tail) {
            pthread_join(prev->thread, NULL);
        }
        free(prev);
    }
        
}

/* Iterate through all threads in the thread pool and if any have terminated, join them to main, remove
 * from the linked list and free the structure. This allows for more threads to then be created. */
void try_join_threads(struct thread_pool *thread_pool) 
{
    struct node *prev = NULL;
    struct node *curr = thread_pool->head->next;
    while (curr != thread_pool->tail) {
        prev = curr;
        curr = curr->next;
        if (pthread_tryjoin_np(prev->thread, NULL) == 0) {
            remove_node(prev);
            free(prev);
        }
    }
}