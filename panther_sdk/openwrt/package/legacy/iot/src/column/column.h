
#ifndef __COLUMN_H__
#define __COLUMN_H__


#include  "common.h"
#include  "myutils/debug.h"


typedef enum      {
	COLUMN_TYPE_NURSERY_RHYMES = 0,//儿歌精选
	COLUMN_TYPE_SINOLOGY_CLASSIC,//国学经典
	COLUMN_TYPE_BEDTIME_STORIES,//睡前故事
	COLUMN_TYPE_FAIRY_TALES, //童话故事
	COLUMN_TYPE_ENGLISH_ENLIGHTTENMENT,//英语启蒙
	COLUMN_TYPE_COLLECTION,//收藏
	COLUMN_TYPE_MAX
} ColumeType;



#define COLUMN_NURSERY_RHYMES_DIR   				"儿歌精选"
#define COLUMN_SINOLOGY_CLASSIC_DIR 				"国学经典"
#define      COLUMN_BEDTIME_STORIES_DIR 			"睡前故事"
#define      COLUMN_FAIRY_TALES_DIR 	 			"童话故事"
#define      COLUMN_ENGLISH_ENLIGHTTENMENT_DIR      "英语启蒙"
#define      COLUMN_COLLECTION_DIR      			"收藏"



#define COLUMN_NURSERY_RHYMES_PLAYLIST   			"nurseryplaylist"
#define COLUMN_SINOLOGY_CLASSIC_PLAYLIST  			"sinologyplaylist"
#define COLUMN_BEDTIME_STORIES_PLAYLIST  			"bedtimeplaylist"
#define COLUMN_FAIRY_TALES_PLAYLIST  	 			"fairyplaylist"
#define COLUMN_ENGLISH_ENLIGHTTENMENT_PLAYLIST 		"englishplaylist"
#define COLUMN_COLLECTION_PLAYLIST       			"collplaylist"

#define COLUMN_NURSERY_RHYMES_SPEECH   				"儿歌哆唻咪"
#define COLUMN_SINOLOGY_CLASSIC_SPEECH  			"国学经典"
#define COLUMN_BEDTIME_STORIES_SPEECH  				"睡前故事"
#define COLUMN_FAIRY_TALES_SPEECH  	 				"童话故事"
#define COLUMN_ENGLISH_ENLIGHTTENMENT_SPEECH   "英语ABC"
#define COLUMN_COLLECTION_SPEECH       			   "开启探索收藏夹的内容"


void SetColumnType(int type) ;
int GetColumnType() ;



#endif








