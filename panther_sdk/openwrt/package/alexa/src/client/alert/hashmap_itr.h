/*
 * hashmap_itr.h
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

#ifndef HASHMAP_ITR_H_
#define HASHMAP_ITR_H_

#include "hashmap.h"

#ifdef __cplusplus
extern "C" {
#endif

struct hashmap_itr_st {
  map_t map;
  size_t index;
};

typedef struct hashmap_itr_st *hashmap_itr_t;

/* create a hashmap iterator, advances to first value is available*/
extern hashmap_itr_t hashmap_iterator(map_t map);

/* returns the value at the current index */
extern  void * hashmap_iterator_value(hashmap_itr_t itr);

/* return 1 is can advance, 0 otherwise */
extern  int hashmap_iterator_has_next(hashmap_itr_t itr);

/*
 * check if iterator can advance, if so advances
 * returns current index if can advance and -1 otherwise
 */
extern int hashmap_iterator_next(hashmap_itr_t itr);

extern void hashmap_iterator_free(hashmap_itr_t itr);



#ifdef __cplusplus
}
#endif


#endif /* HASHSET_ITR_H_ */