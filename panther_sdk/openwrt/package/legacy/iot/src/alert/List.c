
//
// list.c
//
// Copyright (c) 2010 TJ Holowaychuk <tj@vision-media.ca>
//

#include "list.h"
#include "myutils/debug.h"
#include "MusicItemParser.h"

/*
 * Allocate a new list_t. NULL on failure.
 */

list_t *
list_new() {
  list_t *self;
  if (!(self = LIST_MALLOC(sizeof(list_t))))
    return NULL;
  self->head = NULL;
  self->tail = NULL;
  self->free = NULL;
  self->match = NULL;
  self->len = 0;
  return self;
}


/*
 * Allocate a new list_t. NULL on failure.
 */

list_t *
list_new_free(list_free_cb callback) {
  list_t *self;
  if (!(self = LIST_MALLOC(sizeof(list_t))))
    return NULL;
  self->head = NULL;
  self->tail = NULL;
  self->free = callback;
  self->match = NULL;
  self->len = 0;
  return self;
}
/*
 * Allocate a new list_t. NULL on failure.
 */

list_t *
list_new_free_match(list_free_cb free, list_match_cb match) {
  list_t *self;
  if (!(self = LIST_MALLOC(sizeof(list_t))))
    return NULL;
  self->head = NULL;
  self->tail = NULL;
  self->free = free;
  self->match = match;
  self->len = 0;
  return self;
}

/*
 * Free the list.
 */

void
list_destroy(list_t *self) {
  unsigned int len = self->len;
  list_node_t *next;
  list_node_t *curr = self->head;
  //error("len:%d",len);
  while (len--) {
    next = curr->next;
    if (self->free) {
		//error("curr->val:%p",curr->val);
		self->free(curr->val);
	}
    LIST_FREE(curr);//free(curr)
    curr = next;
  }

  LIST_FREE(self);//free(self)
}

/*
 * Append the given node to the list
 * and return the node, NULL on failure.
 */

list_node_t *
list_rpush(list_t *self, list_node_t *node) {
  if (!node) return NULL;

  if (self->len) {
    node->prev = self->tail;
    node->next = NULL;
    self->tail->next = node;
    self->tail = node;
  } else {
    self->head = self->tail = node;
    node->prev = node->next = NULL;
  }

  ++self->len;
  return node;
}

/*
 * Return / detach the last node in the list, or NULL.
 */

list_node_t *
list_rpop(list_t *self) {
  if (!self->len) return NULL;

  list_node_t *node = self->tail;

  if (--self->len) {
    (self->tail = node->prev)->next = NULL;
  } else {
    self->tail = self->head = NULL;
  }

  node->next = node->prev = NULL;
  return node;
}


/*
 * Return 0 success , otherwise failed.
 */
int list_add(list_t *self, void *val)
{
	list_node_t *node =NULL ;
	node = list_node_new(val);//将val的结构体数据放入node中
	return list_rpush(self, node) != NULL ? 0 : 1 ;	
	//再将list_node_t *node的数据放入list_t *self链表中，
	//判断len的长度，并存入后将长度进行计算移位
}

list_node_t *
list_linsert(list_t *self, list_node_t *prev, list_node_t *new) {

	if (!new) return NULL;
	
	if (prev)  {	
		//error("prev:%p");
		//error("new:%p");
		//error("prev->next:%p", prev->next);
		new->prev = prev;
		new->next = prev->next; 
		prev->next = new;
		if(self->tail  == prev) { 
			self->tail = new;			
		}
		++self->len;
	} else {
		//list_rpush(self, new);
		list_lpush(self, new);
		
	}
	return new;
}
list_node_t *
list_rinsert(list_t *self, list_node_t *prev, list_node_t *new) {

	if (!new) return NULL;
	
	if (prev)  {	
		new->prev = prev;
		new->next = prev->next; 
		prev->next = new;
		if(self->tail  == prev) { 
			self->tail = new;			
		}
	
		++self->len;
	} else {
		list_rpush(self, new); 
	}

	return new;
}

/*
 * Return 0 success , otherwise failed.
 */
int list_insert(list_t *self,void *src, void *dst) {
	list_node_t *prev =NULL ;
	list_node_t *next =NULL ;
	if(src) {
		prev = list_find(self, src);
	}
	next = list_node_new(dst);
	return list_linsert(self, prev, next) != NULL ? 0 : 1 ;
}

/*
 * Return / detach the first node in the list, or NULL.
 */

list_node_t *
list_lpop(list_t *self) {
  if (!self->len) return NULL;

  list_node_t *node = self->head;

  if (--self->len) {
    (self->head = node->next)->prev = NULL;
  } else {
    self->head = self->tail = NULL;
  }

  node->next = node->prev = NULL;
  return node;
}

/*
 * Prepend the given node to the list
 * and return the node, NULL on failure.
 */

list_node_t *
list_lpush(list_t *self, list_node_t *node) {
  if (!node) return NULL;

  if (self->len) {
    node->next = self->head;
    node->prev = NULL;
    self->head->prev = node;
    self->head = node;
  } else {
    self->head = self->tail = node;
    node->prev = node->next = NULL;
  }

  ++self->len;
  return node;
}

/*
 * Return the node associated to val or NULL.
 */

list_node_t *
list_find(list_t *self, void *val) { 	
  list_iterator_t *it = list_iterator_new(self, LIST_HEAD);
  list_node_t *node;

  while ((node = list_iterator_next(it))) {
    if (self->match) {
      if (self->match(val, node->val)) {
        list_iterator_destroy(it);
        return node;
      }
    } else {
      if (val == node->val) {
        list_iterator_destroy(it);
        return node;
      }
    }
  }

  list_iterator_destroy(it);
  return NULL;
}

/*
 * Return the node at the given index or NULL.
 * index:	-1-->播放最后一条语音
 * index     1-->播放最前面的一条语音
 */

list_node_t *
list_at(list_t *self, int index) {
  list_direction_t direction = LIST_HEAD;

  if (index < 0) {
    direction = LIST_TAIL;
    index = ~index;//1
  }		
  if ((unsigned)index < self->len) {	//self->len=8
    list_iterator_t *it = list_iterator_new(self, direction);
    list_node_t *node = list_iterator_next(it);
    while (index--) 
    {
		node = list_iterator_next(it);
	}
    list_iterator_destroy(it);
    return node;
  }

  return NULL;
}


/*
 * Return the node at the given index or NULL.
 */

list_node_t *
list_at_value(list_t *self, int index) {
  list_direction_t direction = LIST_HEAD;

  if (index < 0) {
    direction = LIST_TAIL;
    index = ~index;
  }
  if ((unsigned)index < self->len) {
    list_iterator_t *it = list_iterator_new(self, direction);
    list_node_t *node = list_iterator_next(it);
    while (index--) node = list_iterator_next(it);
    list_iterator_destroy(it);
    return node->val;
  }

  return NULL;
}

/*
 * Remove the given node from the list, freeing it and it's value.
 */
/*
void
list_remove(list_t *self, list_node_t *node) {
  node->prev
    ? (node->prev->next = node->next)
    : (self->head = node->next);

  node->next
    ? (node->next->prev = node->prev)
    : (self->tail = node->prev);

  if (self->free) self->free(node->val);

  LIST_FREE(node);
  --self->len;
}*/
void list_node_free(list_t *self , list_node_t *node)
{
	if(node) {
		if (self->free) {
			self->free(node->val);
		}
		LIST_FREE(node);
	}
}

void
list_remove(list_t *self, void *val) {
	list_node_t *node = NULL;
	node = list_find(self, val);
	if(node == NULL){
		return;	
	}
	//info("node:%p",node);
	//info("node->val:%p",node->val);
	node->prev
	 ? (node->prev->next = node->next)
	 : (self->head = node->next);

	node->next
	 ? (node->next->prev = node->prev)
	 : (self->tail = node->prev);

	//LIST_FREE(node);
	list_node_free(self , node);

	--self->len;
}

int list_length(list_t *self) {
	if(self != NULL) 
		return self->len;
	return 0;
}
void
list_show(list_t *self) {
  info("enter");
  unsigned int len = self->len;
  list_node_t *next;
  list_node_t *curr = self->head;
  while (len--) {
    next = curr->next;
   	MusicItem *item = (MusicItem *)curr->val;
  	info("item:%p",item);
	info("item->Title:%s",item->pTitle);
    curr = next;
  }
  info("self->len:%d",self->len);
  info("exit");
}

void
list_clear(list_t *self) {
  unsigned int len = self->len;
  list_node_t *next;
  list_node_t *curr = self->head;
  while (len--) {
    next = curr->next;
    if (self->free) {
		self->free(curr->val);
	}
    LIST_FREE(curr);
    curr = next;
  }
  self->len = 0;
}


