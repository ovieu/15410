/** @file linklist.h
 *  @brief This file defines the type and function prototypes for linked lists.
 *
 *  @author Patrick Koenig (phkoenig)
 *  @author Jack Sorrell (jsorrell)
 *  @bug No known bugs.
 */

#ifndef _LINKLIST_H
#define _LINKLIST_H

#include <kern_common.h>

typedef struct listnode {
    void *data;
    struct listnode *next;
} listnode_t;

typedef struct linklist {
    listnode_t *head;
    listnode_t *tail;
} linklist_t;

/* A comparison function for list elements.  Returns a negative value if data1
 * is less than data2, a positive value if data1 is greater than data2, and 0
 * if they are equal.
 */
typedef int (*data_cmp_t)(void *data1, void *data2);

/* linked list functions */
int linklist_init(linklist_t *list);
void linklist_add_head(linklist_t *list, void *data, listnode_t *node);
void linklist_add_tail(linklist_t *list, void *data, listnode_t *node);
void linklist_add_sorted(linklist_t *list, void *data, data_cmp_t cmp,
  listnode_t *node);
int linklist_peek_head(linklist_t *list, void **data);
int linklist_remove_head(linklist_t *list, void **data, listnode_t **node);
int linklist_remove(linklist_t *list, void *datakey, bool (*eq)(void*, void*),
  void **data, listnode_t **node);
int linklist_rotate_head(linklist_t *list, void **data);
int linklist_rotate_val(linklist_t *list, void *datakey,
  bool (*eq)(void*, void*), void **data);
bool linklist_contains(linklist_t *list, void *data, bool (*eq)(void*, void*));
int linklist_move(linklist_t *oldlist, linklist_t* newlist);
bool linklist_empty(linklist_t *list);

#endif /* _LINKLIST_H */
