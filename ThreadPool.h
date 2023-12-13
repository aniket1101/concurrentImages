#define _GNU_SOURCE
#define __USE_GNU
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>

/* Linked List structure for the thread pool. */
struct thread_pool 
{
    struct node *head;
    struct node *tail;
};

/* Node structure for the thread pool. */
struct node 
{
    struct node *prev;
    struct node *next;
    pthread_t thread;
};

bool initialise_thread_pool(struct thread_pool *thread_pool);
struct node *new_node(pthread_t thread);
bool add_to_thread_pool(struct thread_pool *thread_pool, pthread_t thread);
void remove_node(struct node *node);
void join_threads(struct thread_pool *thread_pool);
void try_join_threads(struct thread_pool *thread_pool);