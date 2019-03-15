#include "SearchMusic.h"

#include <stdio.h>

void FreeSearchKey(search_key_t **key)
{
	if(*key) 
	{
		if((*key)->sence_key) 
		{
			free((*key)->sence_key);
			(*key)->sence_key = NULL;
		}
		if((*key)->song_key.music_name) 
		{
			free((*key)->song_key.music_name);
			(*key)->song_key.music_name = NULL;
		}
		if((*key)->song_key.singer_name) 
		{
			free((*key)->song_key.singer_name);
			(*key)->song_key.singer_name = NULL;
		}
		free(*key);
		(*key) = NULL;
	}
}
