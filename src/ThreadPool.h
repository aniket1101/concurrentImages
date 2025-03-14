#define _GNU_SOURCE
#define __USE_GNU
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>

// Represents a thread pool using a linked-list as a medium
struct t_pool {
    struct node *head; /* Head of the linked list. */
    struct node *tail; /* Tail of the linked list. */
};

struct node {
    struct node *prev; /* Previous node in the linked list. */
    struct node *next; /* Next node in the linked list. */
    pthread_t thread;  /* Thread beloning to the current node. */
};

bool thread_pool_init(struct t_pool *pool);
struct node *create_node(pthread_t thread);
void remove_node(struct node *node);
bool add_thread_to_pool(pthread_t thread, struct t_pool *pool);
void threads_join(struct t_pool *pool);
void tryjoin_threads(struct t_pool *pool);