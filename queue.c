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
    /* TODO: How about freeing the list elements and the strings? */
    /* Free queue structure */
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
    /* TODO: You need to write the complete code for this function */
    /* Remember: It should operate in O(1) time */
    /* TODO: Remove the above comment when you are about to implement. */
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
    /* TODO: You need to write the code for this function */
    /* Remember: It should operate in O(1) time */
    /* TODO: Remove the above comment when you are about to implement. */
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
    /* TODO: You need to write the code for this function */
    /* TODO: Remove the above comment when you are about to implement. */
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
    list_ele_t root = {.next = q->head};
    list_ele_t *tail = NULL;
    for (int k = 1; k < n; k *= 2) {
        tail = &root;
        while (tail->next) {
            // setup next sublists l1 and l2
            list_ele_t *cur = tail, *l1 = cur->next, *l2;
            int n1 = 0;
            int n2 = 0;
            while (cur->next && n1 < k) {
                ++n1;
                cur = cur->next;
            }
            l2 = cur->next;
            while (cur->next && n2 < k) {
                ++n2;
                cur = cur->next;
            }
            list_ele_t *next = cur->next;
            list_ele_t *prevtail = tail;
            prevtail->next = NULL;

            // merge p and q
            while (n1 || n2) {
                if (!n2 || (n1 && strcmp(l1->value, l2->value) < 0)) {
                    if (!prevtail->next)
                        prevtail->next = l1;
                    --n1;
                    tail->next = l1;
                    tail = l1;
                    l1 = l1->next;
                } else {
                    if (!prevtail->next)
                        prevtail->next = l2;
                    --n2;
                    tail->next = l2;
                    tail = l2;
                    l2 = l2->next;
                }
            }
            tail->next = next;
        }
    }
    q->head = root.next;
    q->tail = tail ? tail : q->head;
}
