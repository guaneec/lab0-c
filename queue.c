#include "queue.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "harness.h"

/*
 * Create empty queue.
 * Return NULL if could not allocate space.
 */
queue_t *q_new()
{
    queue_t *q = malloc(sizeof(queue_t));
    if (!q)
        return NULL;
    q->head = NULL;
    q->tail = NULL;
    q->size = 0;
    return q;
}

/* Free all storage used by queue */
void q_free(queue_t *q)
{
    if (!q)
        return;
    list_ele_t *p = q->head;
    while (p) {
        list_ele_t *tmp = p;
        p = p->next;
        free(tmp->value);
        free(tmp);
    }
    free(q);
}

/*
 * Attempt to insert element at head of queue.
 * Return true if successful.
 * Return false if q is NULL or could not allocate space.
 * Argument s points to the string to be stored.
 * The function must explicitly allocate space and copy the string into it.
 */
bool q_insert_head(queue_t *q, char *s)
{
    list_ele_t *newh;
    if (!q)
        return false;
    newh = malloc(sizeof(list_ele_t));
    /* Don't forget to allocate space for the string and copy it */
    if (!newh)
        return false;
    newh->value = strdup(s);
    /* What if either call to malloc returns NULL? */
    if (!newh->value) {
        free(newh);
        return false;
    }
    if (!q->head) {
        q->tail = newh;
    }
    newh->next = q->head;
    q->head = newh;
    ++q->size;
    return true;
}

/*
 * Attempt to insert element at tail of queue.
 * Return true if successful.
 * Return false if q is NULL or could not allocate space.
 * Argument s points to the string to be stored.
 * The function must explicitly allocate space and copy the string into it.
 */
bool q_insert_tail(queue_t *q, char *s)
{
    if (!q)
        return false;
    list_ele_t *e = malloc(sizeof(list_ele_t));
    if (!e)
        return false;
    e->value = strdup(s);
    e->next = NULL;
    if (!e->value) {
        free(e);
        return false;
    }
    if (!q->tail) {
        q->head = q->tail = e;
    } else {
        q->tail->next = e;
    }
    q->tail = e;
    ++q->size;
    return true;
}

/*
 * Attempt to remove element from head of queue.
 * Return true if successful.
 * Return false if queue is NULL or empty.
 * If sp is non-NULL and an element is removed, copy the removed string to *sp
 * (up to a maximum of bufsize-1 characters, plus a null terminator.)
 * The space used by the list element and the string should be freed.
 */
bool q_remove_head(queue_t *q, char *sp, size_t bufsize)
{
    if (!q || !q->head)
        return false;
    if (sp) {
        strncpy(sp, q->head->value, bufsize);
        sp[bufsize - 1] = '\0';
    }
    list_ele_t *e = q->head;
    q->head = q->head->next;
    if (e->value)
        free(e->value);
    free(e);
    if (!q->head)
        q->tail = NULL;
    --q->size;
    return true;
}

/*
 * Return number of elements in queue.
 * Return 0 if q is NULL or empty
 */
int q_size(queue_t *q)
{
    if (!q || !q->head)
        return 0;
    return q->size;
}

/*
 * Reverse elements in queue
 * No effect if q is NULL or empty
 * This function should not allocate or free any list elements
 * (e.g., by calling q_insert_head, q_insert_tail, or q_remove_head).
 * It should rearrange the existing ones.
 */
void q_reverse(queue_t *q)
{
    if (!q || !q->head)
        return;
    list_ele_t *oldhead = q->head;
    list_ele_t *prev = NULL, *cur = q->head, *next;
    while (cur->next) {
        next = cur->next;
        cur->next = prev;
        prev = cur;
        cur = next;
    }
    cur->next = prev;
    q->head = cur;
    q->tail = oldhead;
}

void split_list(list_ele_t *src, list_ele_t **front, list_ele_t **back)
{
    list_ele_t *fast = src->next, *slow = src;
    while (fast) {
        fast = fast->next;
        if (fast) {
            slow = slow->next;
            fast = fast->next;
        }
    }
    *front = src;
    *back = slow->next;
    slow->next = NULL;
}

list_ele_t *merge_sorted(list_ele_t *a, list_ele_t *b)
{
    list_ele_t root = {.next = NULL};
    list_ele_t *tail = &root;
    while (a || b) {
        list_ele_t **c =
            (!b || (a && strcmp(a->value, b->value) <= 0)) ? &a : &b;
        if (!(root.next))
            root.next = *c;
        tail = tail->next = *c;
        *c = (*c)->next;
    }
    return root.next;
}

// adapted from https://www.geeksforgeeks.org/merge-sort-for-linked-list/
// my version was just a bit too slow :'(
void mergesort(list_ele_t **headref)
{
    list_ele_t *head = *headref;
    list_ele_t *a, *b;
    if (!head || !(head->next)) {
        return;
    }
    split_list(head, &a, &b);
    mergesort(&a);
    mergesort(&b);
    *headref = merge_sorted(a, b);
}

/*
 * Sort elements of queue in ascending order
 * No effect if q is NULL or empty. In addition, if q has only one
 * element, do nothing.
 */
void q_sort(queue_t *q)
{
    // merge sort
    int n = q_size(q);
    if (!n)
        return;
    mergesort(&q->head);
    list_ele_t *c;
    for (c = q->head; c->next; c = c->next)
        ;
    q->tail = c;
}
