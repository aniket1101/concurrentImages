#include "ThreadPool.h"

bool thread_pool_init(struct t_pool *t_pool) {
    t_pool->head = malloc(sizeof(struct node));
    t_pool->tail = malloc(sizeof(struct node));
    if (t_pool->head == NULL || t_pool->tail == NULL) {
        return false;
    }

    t_pool->head->prev = NULL;
    t_pool->head->next = t_pool->tail;
    t_pool->tail->prev = t_pool->head;
    t_pool->tail->next = NULL;

    return true;
}