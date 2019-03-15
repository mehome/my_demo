#ifndef __LIST_ITERATOR_H__
#define __LIST_ITERATOR_H__

#include "list.h"

extern  list_iterator_t *list_iterator_new(list_t *list, list_direction_t direction);

extern int list_iterator_has_next(list_iterator_t *self);

extern void * list_iterator_next_value(list_iterator_t *self);

extern void list_iterator_destroy(list_iterator_t *self);
#endif