
#include "CommsatKeyEmotion.h"
#include "commsat.h"
#include "debug.h"

typedef struct __func_proc_t {
    int code;
    json_object *(*proc)(json_object *cmd);
} FuncProc;

static      FuncProc procTables[] = {
		{TURING_FUNC_CHAT, NULL},
		{TURING_SLEEP_CONTROL, NULL},
		{TURING_VOLUME_CONTROL, VolumeControlFunc},
		{TURING_WEATHER_INQUIRY, NULL},
		{TURING_TIME_INQUIRY, NULL},
		{TURING_DATE_INQUIRY, NULL},
		{TURING_COUNT_CALCLATE, NULL},
		{TURING_MUSIC_PLAY, MusicPlayFunc},
		{TURING_STORY_TELL, StoryTellFunc},
		{TURING_POEMS_RECITE,PoemsReciteFunc},
		{TURING_POETRY_INTER, NULL},
		{TURING_ANIMAL_SOUND, AnimalSoundFunc},
		{TURING_ENCYCLOPEDIC_KNOWLEDGE, NULL},
		{TURING_PHONE_CALL, PhoneCallFunc},
		{TURING_SOUND_GUESS, NULL},
		{TURING_ACTYIVE_INTER, NULL},
		{TURING_MAEKED_WORDS, NULL},
		{TURING_CHINESE_ENGLISH, NULL},
		{TURING_ROBOT_DANCE, NULL},
		{0, NULL},
};
json_object * VolumeControlFunc(json_object *cmd)
{
	json_object *val = NULL;
	json_object *keyWord = NULL;
	int operate = 0;
	int arg =0;
	
	keyWord = json_object_new_object();

	val = json_object_object_get(cmd, "operate");
	if(val)
		operate = json_object_get_int(val);

	val = json_object_object_get(cmd, "arg");
	if(val)
		arg = json_object_get_int(val);


	json_object_object_add(keyWord, "operate", json_object_new_int(operate));


	json_object_object_add(keyWord, "arg", json_object_new_int(arg));

		
	return keyWord;
}
json_object * MusicPlayFunc(json_object *cmd)
{
	json_object *keyWord = NULL;
	
	json_object *val = NULL;
	char *singer = NULL, *title =NULL;

	keyWord = json_object_new_object();
	
	val = json_object_object_get(cmd, "singer");
	if(val)
		singer = json_object_get_string(val);

	val = json_object_object_get(cmd, "title");
	if(val)
		title = json_object_get_string(val);

	if(singer)
		json_object_object_add(keyWord, "singer", json_object_new_string(singer));
	else
		json_object_object_add(keyWord, "singer", json_object_new_string(""));
	
	if(title)
		json_object_object_add(keyWord, "title", json_object_new_string(title));
	else
		json_object_object_add(keyWord, "title", json_object_new_string(""));

	return keyWord;
}


json_object * StoryTellFunc(json_object *cmd)
{
	json_object *keyWord = NULL;
	
	json_object *val = NULL;
	char *author = NULL, *title =NULL;
	keyWord = json_object_new_object();
	
	val = json_object_object_get(cmd, "author");
	if(val)
		author = json_object_get_string(val);

	val = json_object_object_get(cmd, "title");
	if(val)
		title = json_object_get_string(val);

	if(author)
		json_object_object_add(keyWord, "author", json_object_new_string(author));
	else
		json_object_object_add(keyWord, "author", json_object_new_string(""));
	if(title)
		json_object_object_add(keyWord, "title", json_object_new_string(title));
	else
		json_object_object_add(keyWord, "title", json_object_new_string(""));

	return keyWord;
	
}
json_object * PoemsReciteFunc(json_object *cmd)
{
	json_object *keyWord = NULL;
		
	json_object *val = NULL;
	char *author = NULL, *title =NULL;

	keyWord = json_object_new_object();
	
	val = json_object_object_get(cmd, "author");
	if(val)
		author = json_object_get_string(val);

	val = json_object_object_get(cmd, "title");
	if(val)
		title = json_object_get_string(val);

	if(author)
		json_object_object_add(keyWord, "author", json_object_new_string(author));
	else
		json_object_object_add(keyWord, "author", json_object_new_string(""));
	if(title)
		json_object_object_add(keyWord, "title", json_object_new_string(title));
	else
		json_object_object_add(keyWord, "title", json_object_new_string(""));



	return keyWord;


}

json_object * AnimalSoundFunc(json_object *cmd)
{
	

	json_object *keyWord = NULL;
	
	json_object *val = NULL;
	char *animal = NULL;

	keyWord = json_object_new_object();
	
	val = json_object_object_get(cmd, "animal");
	if(val)
		animal = json_object_get_string(val);

	
	if(animal)
		json_object_object_add(keyWord, "animal", json_object_new_string(animal));
	else
		json_object_object_add(keyWord, "animal", json_object_new_string(""));
	
	return keyWord;

}

json_object * PhoneCallFunc(json_object *cmd)
{
	
	json_object *keyWord = NULL;

	json_object *val = NULL;
	char *name = NULL, *pNum =NULL;
	
	keyWord = json_object_new_object();
	
	val = json_object_object_get(cmd, "name");
	if(val)
		name = json_object_get_string(val);

	val = json_object_object_get(cmd, "pNum");
	if(val)
		pNum = json_object_get_string(val);

	if(name)
		json_object_object_add(keyWord, "name", json_object_new_string(name));
	else
		json_object_object_add(keyWord, "name", json_object_new_string(""));
	if(pNum)
		json_object_object_add(keyWord, "pNum", json_object_new_string(pNum));
	else
		json_object_object_add(keyWord, "pNum", json_object_new_string(""));

	return keyWord;
}

int ParseKeyWordAndSend(int code , json_object *func)
{
	int ret;
	if(func == NULL)
		return -1;
 	json_object *root = NULL, *keyWord =NULL;
	char *data = NULL;
	root =  json_object_new_object();
	info("func:%s",json_object_to_json_string(func));
	
	int tblLen = sizeof(procTables) / sizeof(procTables[0]);

	int i = 0;
	for(i = 0; i < tblLen; i++) 
	{
		if(code == procTables[i].code) 
		{
			
			if(procTables[i].proc != NULL) {
				keyWord = procTables[i].proc(func);
			} 
			break;
			
		}
	}
	json_object_object_add(root, "code", json_object_new_int(code));


	if(keyWord) {
		json_object_object_add(root, "keyWord", keyWord);
	};
	data = json_object_to_json_string(root);

	OnWriteMsgToCommsat(KEY_WORD,data);
	json_object_put(root);
	return 0;
	
}





