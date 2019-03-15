#include <stdio.h>

static list_t *playlist;

void PlaylistManagerInit()
{
	if (NULL == playlist)
	{
		playlist = list_new();
	}
}

static int ClearPlayList()
{
	if (NULL == playlist) 
	{
		return -1;
	}
	else if (NULL != playlist)
	{
		list_iterator_t *it = NULL ;
		PLAY_DIRECTIVE_STRUCT *playDirective = NULL;
		DEBUG_PRINTF("<<<<<<<<<<<<<<< list_length:%d", list_length(playlist));
		it = list_iterator_new(playlist,LIST_HEAD);
		while(list_iterator_has_next(it))  // 从链表中读出数据，这里并没有删除，后面需要自己删除
		{
			DEBUG_PRINTF("<<<<<<<<<<<<<<< list_length:%d", list_length(playlist));
			playDirective =  list_iterator_next_value(it);
			if(playDirective != NULL) 
			{
				// 删除数据
				list_node_t * node_t = list_find(playlist, playDirective);
				if (node_t != NULL)
				{
					list_remove(playlist, node_t);
					if(playDirective != NULL)
					{
						if (playDirective->m_PlayUrl)
						{
							free(playDirective->m_PlayUrl);
							playDirective->m_PlayUrl = NULL;
						}
						if (playDirective->m_PlayToken)
						{
							free(playDirective->m_PlayToken);
							playDirective->m_PlayToken = NULL;
						}
						free(playDirective);
						playDirective = NULL;
					}
				}
			}
		}
		list_iterator_destroy(it);		
	}
}


// 将流添加到当前队列的末尾
void EnqueuePlaylist(PLAY_DIRECTIVE_STRUCT *playDirective)
{
	list_add(playlist, playDirective);
}

// 清空列表，并播放当前流
void RelaceAllPlaylist(PLAY_DIRECTIVE_STRUCT *playDirective)
{
	ClearPlayList();
	EnqueuePlaylist(playDirective);
}

// 替换队列中的所有流,这不影响当前的播放流。
void ReplaceEnqueuedPlaylist()
{
	
}

// 获取播放列表的长度
int GetPlaylistLength()
{
	return list_length(playlist);
}



