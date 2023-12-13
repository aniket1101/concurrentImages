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

struct node *create_node(pthread_t thread) {
    struct node *node = malloc(sizeof(struct node));
    if (node == NULL) {
        return NULL;
    }

    node->thread = thread;
    return node;
}

void remove_node(struct node *node) {
    node->prev->next = node->next;
    node->next->prev = node->prev;
    free(node);
}

bool add_thread_to_pool(pthread_t *thread, struct t_pool *pool) {
    struct node *node = create_node(thread);

    if (node == NULL) {
        return false;
    }
    pool->tail->prev->next = node;
    node->prev = pool->tail->prev;
    node->next = pool->tail;
    pool->tail->prev = node;
    return true;
}

void threads_join(struct t_pool *pool) {
    struct node *prev = NULL;
    struct node *curr = pool->head;

    while (curr != NULL) {
        prev = curr;
        curr = curr->next;

        if (prev != pool->head && prev != pool->tail) {
            pthread_join(prev->thread, NULL);
        }
        free(prev);
    }
}

void tryjoin_threads(struct t_pool *pool) {
    struct node *prev = NULL;
    struct node *curr = pool->head->next;

    while (curr != pool->tail) {
        prev = curr;
        curr = curr->next;

        if (pthread_tryjoin_np(prev->thread, NULL) == false) {
            remove_node(prev);
            free(prev);
        }
    }
} 