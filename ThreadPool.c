#define _GNU_SOURCE
#define __USE_GNU
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include "ThreadPool.h"

// Initalise a thread pool by assigning basic members
bool thread_pool_init(struct t_pool *t_pool) {
    t_pool->head = (struct node*) malloc(sizeof(struct node));
    t_pool->tail = (struct node*) malloc(sizeof(struct node));

    // Check that neither malloc failed and exit if any did
    if (t_pool->head == NULL || t_pool->tail == NULL) {
        return false;
    }

    /* Return a linked list with a head, a tail and no other nodes
       Ensure that the head can have no node before it and the tail
       can have no node after. */

    t_pool->head->prev = NULL;
    t_pool->head->next = t_pool->tail;
    t_pool->tail->prev = t_pool->head;
    t_pool->tail->next = NULL;

    return true;
}

// Create a new node and assign its thread to the thread passed in
struct node *create_node(pthread_t thread) {
    struct node *node = (struct node*) malloc(sizeof(struct node));

    // Check that malloc has not failed and return a NULL node if it has
    if (node == NULL) {
        return NULL;
    }

    node->thread = thread;
    return node;
}

// Remove a specified node from the thread pool
void remove_node(struct node *node) {

    // Assign the previous node's next node to the current node's next node
    node->prev->next = node->next;

    // Assign the next node's previous node to the current node's previous node
    node->next->prev = node->prev;

    // Free resources
    free(node);
}

// Add a thread to a thread pool
bool add_thread_to_pool(pthread_t thread, struct t_pool *pool) {
    struct node *node = create_node(thread);

    // Check if the node is null (malloc failed)
    if (node == NULL) {
        return false;
    }

    // Add node containing thread to the back of the list
    pool->tail->prev->next = node;
    node->prev = pool->tail->prev;
    node->next = pool->tail;
    pool->tail->prev = node;
    return true;
}


void threads_join(struct t_pool *pool) {
    struct node *prev = NULL;
    struct node *curr = pool->head;

    // Iterate through all threads until hitting a NULL
    while (curr != NULL) {
        prev = curr;
        curr = curr->next;

        // The current node being checked is not the head or tail  
        if (prev != pool->head && prev != pool->tail) {
            // Waits for a thread to terminate and then detaches the thread
            pthread_join(prev->thread, NULL);
        }

        // Free resources
        free(prev);
    }
}

void tryjoin_threads(struct t_pool *pool) {
    struct node *prev = NULL;
    struct node *curr = pool->head->next;

    // Iterate through all nodes in the pool
    while (curr != pool->tail) {
        prev = curr;
        curr = curr->next;

        // Perform a join and check thread's successful termination
        if (pthread_tryjoin_np(prev->thread, NULL) == false) {
            //Remove the current node being checked from the pool and free it
            remove_node(prev);
            free(prev);
        }
    }
} 