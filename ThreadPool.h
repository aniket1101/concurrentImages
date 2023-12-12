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