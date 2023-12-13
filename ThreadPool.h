#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

struct t_pool {
    struct node *head;
    struct node *tail;
};

struct node {
    struct node *prev;
    struct node *next;
    pthread_t thread;
};

bool thread_pool_init(struct t_pool *t_pool);
struct node *create_node(pthread_t thread);
void remove_node(struct node *node);
bool add_thread_to_pool(pthread_t *thread, struct t_pool *pool);
void threads_join(struct t_pool *pool);
void tryjoin_threads(struct t_pool *pool);