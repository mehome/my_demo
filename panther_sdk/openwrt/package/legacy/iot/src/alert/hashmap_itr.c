#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "hashmap_itr.h"

hashmap_itr_t hashmap_iterator(map_t map)
{
  hashmap_itr_t itr = calloc(1, sizeof(struct hashmap_itr_st));
  if (itr == NULL)
    return NULL;

  itr->map = map;
  itr->index = 0;

  /* advance to the first item if one is present */
 // if (hashmap_length(map) > 0)
     // hashmap_iterator_next(itr);
	
  return itr;
}

int hashmap_iterator_has_next(hashmap_itr_t  itr)
{
   size_t index = 0;
	hashmap_map *map = (hashmap_map*)itr->map;
//	printf("%s:%d index=%d\n", __func__, __LINE__, index);
	//printf("%s:%d itr->index=%d len=%d\n", __func__, __LINE__, itr->index, hashmap_length(map));		
	//if(itr->index >= map->table_size) {			
		//printf("%s:%d itr->index >= map->table_size\n", __func__, __LINE__);	
	//} 
	//else 	
		//printf("%s:%d itr->index < map->table_size\n", __func__, __LINE__);
  /* empty or end of the map */
  if (hashmap_length(map) == 0 || itr->index >= map->table_size )
    return 0;

  /* peek to find another entry */
  if(itr->index == 0)  
  	index=0;  
  else   	
  	index = itr->index + 1; 
  while(index <=  map->table_size-1) {
  	if(map->data[index].in_use == 1)  {
		//printf("%s:%d index : %d\n", __func__, __LINE__, index);
  		 return 1;
  	}
  	index++;
  }
//printf("%s:%d index : %d\n", __func__, __LINE__, index);
  /* Otherwise */
  return 0;

}


int hashmap_iterator_next(hashmap_itr_t itr)
{
	//int index;
  //if( hashmap_iterator_has_next(itr) == 0)
  // 	return -1; /* Can't advance */
	hashmap_map * map = (hashmap_map*)itr->map;

   itr->index++;
	while(itr->index <= map->table_size-1 ) {
		if(map->data[itr->index].in_use == 1) {	
			return itr->index;		
		}
		 itr->index++;
	}
	

  return -1;
}

void * hashmap_iterator_value(hashmap_itr_t itr) {

  /* Check to verify we're not at a null value, this can happen if an iterator is created
   * before items are added to the set.
   */
  hashmap_map *map = (hashmap_map*)itr->map;
  if( map->data[itr->index].in_use != 1)
  {
    hashmap_iterator_next(itr);
  }

  return map->data[itr->index].data;
  
}

char *hashmap_iterator_key(hashmap_itr_t itr)	
{

  /* Check to verify we're not at a null value, this can happen if an iterator is created
   * before items are added to the set.
   */
  hashmap_map *map = (hashmap_map*)itr->map;
  if( map->data[itr->index].in_use != 1)
  {
    hashmap_iterator_next(itr);
  }

  return map->data[itr->index].key;
  
}

void hashmap_iterator_free(hashmap_itr_t itr)
{
	if(NULL != itr) {
		free(itr);
		itr = NULL;
	}
}
