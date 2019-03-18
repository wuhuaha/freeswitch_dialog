/*************************************************    
Authors:        wuhuaha
mail:			cn_wwangtao@163.com
Description:    make phone and dialog by esl for freeswitch.                
History:       
    1. Date:    2019/3/15
       Author:  wuhuaha
       Modification:   v1.0
*************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <esl.h>
#include <time.h>
#include "uniu_list.h"
#include "uniu_esl.h"

#define TEST 1

//{辅助性独立工具函数，与系统逻辑无关
/*
*@Destription:generate random string 
*@param:str ; where string save to
*@param:length; length of random string
*@return: success:0  fild:-1
*/
int generate_string(char *str, int length)
{
    register int i,flag;
    if(str == NULL){
        esl_log(ESL_LOG_ERROR,"str is NULL !\n");
        return  -1;
    }
    if(length <= 0){
        esl_log(ESL_LOG_ERROR,"length is 0 !\n");
        *str = 0;
        return -1;
    }
    srand(time(NULL));//通过时间函数设置随机数种子，使得每次运行结果随机。
    for(i = 0; i < length; i ++)
    {
		flag = rand()%3;
		switch(flag)
		{
		case 0:
			str[i] = rand()%26 + 'a'; 
			break;
		case 1:
			str[i] = rand()%26 + 'A'; 
			break;
		case 2:
			str[i] = rand()%10 + '0'; 
			break;
		}
    }
    str[i] = 0;
    esl_log(ESL_LOG_INFO,"%s\n", str);//打印生成的随机数。     
    return 0;
}
/*
*@Destription:malloc a string ,and copy src to it
*@param:src;
*@return: success:dst  faild:NULL
*/
inline char *string_malloc_copy(char *src)
{
    register int i;
    if(src == NULL){
        //printf("src string is NULL\n");
        return NULL;
    }
    int length = strlen(src);
    char *dst = (char *)malloc(length + 1);
    if(dst == NULL){
        esl_log(ESL_LOG_ERROR, "MALLOC FAILD ~~ \n");
        return NULL;
    }
    for(i = 0; i < length; i++)
    {
        dst[i] = *src++;
    }
    dst[i] = 0;
    return dst;    
}

/*
*@Description: 获取wav文件的时长，单位：毫秒
*@param:file_name: what file we compute
*@retrun success:wav time length of wav file; faild: -1  
*/
inline double get_wav_time_length(char* file_name)
{
	double len = 0.0;
 
	if (file_name != NULL)
	{
		FILE* fp;
		fp = fopen(file_name, "rb");
		if (fp != NULL)
		{
			int i;
			int j;
			fseek(fp, 28, SEEK_SET);
			fread(&i, sizeof(i), 1, fp);
			fseek(fp, 40, SEEK_SET);
			fread(&j, sizeof(j), 1, fp);		
 
			fclose(fp);
			fp = NULL;
 
			len = (double)j*1000/(double)i;
		}else{
            esl_log(ESL_LOG_ERROR, "file[%s] open faild\n", file_name);
            return -1;
        }
	}
 
	return len;
}

/*@Destription compute the sub of two timeval (t1 - t2 ,return  n ms)
*/
inline int timeval_sub(struct timeval t1, struct timeval t2)
{
    return ((t1.tv_sec - t2.tv_sec) * 1000000 + t1.tv_usec - t2.tv_usec) / 1000;
}

//}}


//{{  基础操作函数，包括拨打电话、播放wav文件、开始录音

/*
*@Destription:make a call (get info from  config)
*@param:config;
*@return: success:0  fail :-1
*/
int make_call(sip_config_t config)
{
    char call_string[1024];
    char sip_string[128];  //example: sip:1001@192.168.1.1:5060  or  user/1001
    char uuid_string[128];
    char codec_string[64];
	char api_string[512];
    char caller_id_string[128];  

	//标准化sip_string为标准格式，在有domain时使用domain呼叫，否则认为是本机注册用户
    if( config->info->domain != NULL){
        snprintf(sip_string, sizeof(sip_string), "sofia/external/sip:%s%s@%s", (config->info->phone_prefix == NULL ? "" : config->info->phone_prefix), config->info->phone_number, config->info->domain);
    }else{
        snprintf(sip_string, sizeof(sip_string), "user/%s%s", (config->info->phone_prefix == NULL ? "" : config->info->phone_prefix), config->info->phone_number);
    }

    if(config->info->codec != NULL){
        snprintf(codec_string, sizeof(codec_string), "{absolute_codec_string=%s}", config->info->codec);    
    }else{
        *codec_string = 0;
    }

    if(config->uuid != NULL){
        snprintf(uuid_string, sizeof(uuid_string), "{origination_uuid=%s}", config->uuid);
    }else{
        esl_log(ESL_LOG_ERROR,"None uuid\n");
    	char uuid_generate[64];
        generate_string(uuid_generate, 16);
        snprintf(uuid_string, sizeof(uuid_string), "{origination_uuid=%s}", uuid_generate);
        esl_log(ESL_LOG_INFO,"create uuid[%s]\n",uuid_generate);

    }

    if(config->info->caller_id != NULL){
        snprintf(caller_id_string, sizeof(caller_id_string), "{origination_caller_id_number=%s}{origination_caller_id_name=%s}", config->info->caller_id, config->info->caller_id);    
    }else{
        *caller_id_string = 0;
    }

	if(config->info->api_cmd != NULL){
        snprintf(api_string, sizeof(api_string), "%s", config->info->api_cmd);    
    }else{
        snprintf(api_string, sizeof(api_string), "&park");;
    }

    snprintf(call_string, sizeof(call_string), "bgapi originate %s%s%s%s %s", uuid_string, caller_id_string, codec_string, sip_string, api_string);
    esl_log(ESL_LOG_INFO,"%s\n", call_string);
    esl_send(config->handle, call_string);

	if(config->status_cb->makecall_cb != NULL)
	{
		config->status_cb->makecall_cb();
	}

    return 0;
}

/*
*@Destription:播放指定的文件
*@param:file_name:文件名
*@param:config
*@return: success:0 fail:-1
*/
inline int play_wav_file(char *file_name, sip_config_t config, esl_handle_t *handle)
{
    if(file_name == NULL){
        esl_log(ESL_LOG_ERROR, "file_name is NULL");
        return -1;
    }
    if(config == NULL){
        esl_log(ESL_LOG_ERROR, "sip_config_t is NULL");
        return -1;
    }
    if(handle == NULL){		
    	esl_log(ESL_LOG_INFO, "[%s]in play wav file function", file_name);
    	esl_execute(config->handle, "playback", file_name, config->uuid);
    }else{
		esl_log(ESL_LOG_INFO, "[%s]in play wav file function", file_name);
    	esl_execute(handle, "playback", file_name, config->uuid);
	}
   
    return 0;
}

/*
*@Destription:播放指定时长的静音
*@param:play_silence_time_ms:播放静音的时长
*@param:config
*@return: success:0 fail:-1
*/
inline int play_silence(int play_silence_time_ms, sip_config_t config, esl_handle_t *handle)
{
	if(config == NULL){
        esl_log(ESL_LOG_ERROR, "sip_config_t is NULL");
        return -1;
    }
    esl_handle_t *play_handle;
    if(handle == NULL){
        play_handle = config->handle;
    }else{
        play_handle = handle;
    }
	char play_silence_cmd[63];
	snprintf(play_silence_cmd, strlen(play_silence_cmd), "silence_stream://%d", play_silence_time_ms);
    esl_log(ESL_LOG_INFO, "play [%d]ms silence stream(%s)\n", play_silence_time_ms, play_silence_cmd);
    esl_execute(play_handle, "playback", play_silence_cmd, config->uuid);
    return 0;
}

/*
*@Destription:进行录音
*@config;
*@return: success:0 fail:-1
*/
int record_call(sip_config_t config)
{
    char record_file[1024];
    if(config == NULL){
        esl_log(ESL_LOG_ERROR, "sip_config_t is NULL");
        return -1;
    }
    if(config->info->record_path == NULL){
        config->info->record_path = string_malloc_copy("/home/record_path");
        esl_log(ESL_LOG_INFO, "record_path is NULL,so we generate it to[%s]\n", config->info->record_path);
    }
    if(config->info->record_file_name == NULL){
        snprintf(record_file, 1024, "%s.wav", config->uuid);
        config->info->record_file_name = string_malloc_copy(record_file);
        esl_log(ESL_LOG_INFO, "record_file_name is NULL,so we generate it to[%s]\n", config->info->record_file_name);
    }
    if(*(config->info->record_path + strlen(config->info->record_path) - 1 ) == '/'){
        snprintf(record_file, 1024, "%s%s", config->info->record_path, config->info->record_file_name);
    }else{
        snprintf(record_file, 1024, "%s/%s", config->info->record_path, config->info->record_file_name);
    }
    esl_execute(config->handle, "mkdir", config->info->record_path, config->uuid);
    esl_log(ESL_LOG_INFO, "record_file_name is %s\n", record_file);
    esl_execute(config->handle, "record_session", record_file, config->uuid);
    return 0;
}

//}}


//{{ 当前拨打状态（包括正在播放的音频文件名、文件大小、播放开始时间等）调整维护相关函数；
//   包含：设置拨打状态、重置拨打状态、打断当前播放的文件

/*
*@Destription:设置正在拨打的音频状态
*@param:file_name:拨打的文件名
*@param:config
*@return: success:0  fail:-1
*/
inline int set_playing_file_status(char *file_name, sip_config_t config)
{
    if(file_name == NULL){
        esl_log(ESL_LOG_ERROR, "file_name is NULL");
        return -1;
    }
    if(config == NULL){
        esl_log(ESL_LOG_ERROR, "sip_config_t is NULL");
        return -1;
    }
    sprintf(config->playing_file_status.play_file_name, "%s", file_name);
    gettimeofday(&(config->playing_file_status.play_start_time), NULL);
    config->playing_file_status.play_file_length_ms = get_wav_time_length(config->playing_file_status.play_file_name);
    return 0;
}

/*
*@Destription:将正在拨打的音频重置掉
*@param:config
*@return: success:0  fail:-1
*/
int reset_playing_file_status(sip_config_t config)
{
    if(config == NULL){
        esl_log(ESL_LOG_ERROR, "sip_config_t is NULL");
        return -1;
    }
    *(config->playing_file_status.play_file_name) = 0;
    config->playing_file_status.play_file_length_ms = 0;
    return 0;
}

/*
*@Destription:when playing file last time less than protect(ms), clean play list, else, break it and clean play list
*@param:config;
*@protect:protect time (ms) of  playing file
*@return: success:0  fail :-1
*/
int break_playing_file(sip_config_t config, int protect)
{
    if(config == NULL){
        esl_log(ESL_LOG_ERROR, "sip_config_t is NULL");
        return -1;
    }
    char cmd_str[256];
    struct timeval now;
    gettimeofday(&now,NULL);
    int remain_time = config->playing_file_status.play_file_length_ms - timeval_sub(now, config->playing_file_status.play_start_time);
    esl_log(ESL_LOG_INFO, "playing file[%s](whole length [%lf]ms);now played time:[%d]((%d - %d)*1000 + (%d - %d)/1000);remain_time:[%lf]ms\n", 
                config->playing_file_status.play_file_name, config->playing_file_status.play_file_length_ms, 
                    timeval_sub(now, config->playing_file_status.play_start_time), now.tv_sec, config->playing_file_status.play_start_time.tv_sec, now.tv_usec, config->playing_file_status.play_start_time.tv_usec,
                        remain_time);
    if(remain_time > protect){
        snprintf(cmd_str, sizeof(cmd_str), "bgapi uuid_break %s", config->uuid);
        esl_send_recv(config->handle, cmd_str); 
        esl_log(ESL_LOG_INFO, "break playing file[%s]\n",config->playing_file_status.play_file_name);
        esl_log(ESL_LOG_INFO, "%s\n",config->handle->last_sr_reply);
        reset_playing_file_status(config);
    }
    link_list_clear_free(config->play_list);
    esl_log(ESL_LOG_INFO, "clear and free all node in the play_list\n"); 
    return 0;
}

/*
*@Description:get uuid from freeswitch
*@param:config
*@retrun: success:0  fail:-1
*/
int get_uuid(sip_config_t config)
{
	esl_send_recv(config->handle, "api create_uuid\n\n");
	if (config->handle->last_sr_event && config->handle->last_sr_event->body)
	{
		config->uuid = string_malloc_copy(config->handle->last_sr_event->body);
		return 0;
	}
	else
	{
		return -1;
	}
	return -1;
}

/*
*@Description:hangup call
*@param:config
*@retrun: success:0  fail:-1
*/

int active_hangup(sip_config_t config)
{
	char hangup_cmd[128] = { 0 };
	sprintf(hangup_cmd, "bgapi uuid_kill %s\n\n", config->uuid);
	esl_send_recv(config->handle, hangup_cmd);
	if (config->handle->last_sr_event && config->handle->last_sr_event->body)
	{
		esl_log(ESL_LOG_INFO,"[%s]\n", config->handle->last_sr_event->body);
		return 0;
	}
	else
	{
		esl_log(ESL_LOG_INFO,"[%s] last_sr_reply\n", config->handle->last_sr_reply);
		return -1;
	}
	return -1;
}

//}}


//{{播放列表维护和取出相关函数；包括：音频文件加入链表、从链表取出文件、清空链表

/*@Description: Add file to play_list 将音频文件加入到播放列表中
*@param:file_name ; what adding into list
*@param:config ; uniu config
*@param:if_last ; if add into last or first
*@pfaram:if_clean; if clean play_list
*@return: success:0 error:-1
*/
int add_to_playlist(char *file_name, sip_config_t config, int if_last, int if_clean)
{
    link_t play_list = config->play_list;
    if(play_list == NULL){
        esl_log(ESL_LOG_INFO, "play_list is not init!now init it~\n");
        play_list = new_link_list();
    }
    if(file_name == NULL){
        esl_log(ESL_LOG_ERROR, "file_name is NULL\n");
        return -1;
    }
    if(if_clean){
        link_list_clear_free(play_list);
        esl_log(ESL_LOG_INFO, "clear and free all node and all value in the play_list\n"); 
    }
    char *file = string_malloc_copy(file_name);
    esl_log(ESL_LOG_INFO, "file:%s\n",file);
    if(file == NULL){
        esl_log(ESL_LOG_ERROR, "file_name malloc error,exit!\n");
        return -1;
    }
    if(if_last){
        link_list_add_last(play_list, file);
    }else{
        link_list_add_first(play_list, file);
    }
    return 0;
}

/*@Description: Add silence stream to play_list 将音频文件加入到播放列表中
*@param:play_silence_time_ms ; how long silence adding into list
*@param:config ; uniu config
*@param:if_last ; if add into last or first
*@pfaram:if_clean; if clean play_list
*@return: success:0 error:-1
*/
int add_silence_to_playlist(int play_silence_time_ms, sip_config_t config, int if_last, int if_clean)
{
    link_t play_list = config->play_list;
    if(play_list == NULL){
        esl_log(ESL_LOG_INFO, "play_list is not init!now init it~\n");
        play_list = new_link_list();
    }
    char file_name[128];
	snprintf(file_name, 128, "silence_stream://%d", play_silence_time_ms);

    if(play_silence_time_ms <= 0){
        esl_log(ESL_LOG_ERROR, "play_silence_time_ms is 0\n");
    }
    if(if_clean){
        link_list_clear_free(play_list);
        esl_log(ESL_LOG_INFO, "clear and free all node and all value in the play_list\n"); 
    }
    char *file = string_malloc_copy(file_name);
    esl_log(ESL_LOG_INFO, "file_name[%s],file:[%s]\n",file_name, file);
    if(file == NULL){
        esl_log(ESL_LOG_ERROR, "file_name malloc error,exit!\n");
        return -1;
    }
    if(if_last){
        link_list_add_last(play_list, file);
    }else{
        link_list_add_first(play_list, file);
    }
    return 0;
}

/*
*Description:get file from the head of play_list, remove it from list and free it 
*param:config
*param:play_file_name, the file we get
*return:success: file_name fail:NULL
*/
char *get_from_playlist(sip_config_t config, char *play_file_name)
{

    if(!config->play_list->size){
        esl_log(ESL_LOG_ERROR, "none file in list\n");
        play_file_name = NULL;
        return NULL;
    }
    char *value = link_list_get(config->play_list, 0);
    if(value != NULL){
        sprintf(play_file_name, "%s", value);
        SAFE_FREE(value);
    }else{
        play_file_name = NULL;
    }
    link_list_remove_first(config->play_list);
    return play_file_name;
}

/*
*@Destription:从列表头部获取音频文件并播放
*@config;
*@return: success:0 fail:-1
*/
int play_form_list(sip_config_t config)
{
    if(config == NULL){
        esl_log(ESL_LOG_ERROR, "sip_config_t is NULL");
        return -1;
    }
    char play_file[512];
    if(config->play_list->size && !(*(config->playing_file_status.play_file_name)))
    {
            esl_log(ESL_LOG_INFO, "there are [%d] files in play list , uuid[%s], ready to play ~ ~\n", config->play_list->size, config->uuid);
            get_from_playlist(config, play_file);
            esl_log(ESL_LOG_INFO, "get file [%s] from play list ~ \n", play_file);
            play_wav_file(play_file, config, config->play_handle);
    }
    return 0;
}

/*
@Destription:clear play_list and free all value in it
@param: config
@return: success:0 fail:-1
*/
inline int clear_free_playlist(sip_config_t config)
{
    if(config == NULL){
        esl_log(ESL_LOG_ERROR, "sip_config_t is NULL");
        return -1;
    }
	link_list_clear_free(config->play_list);
	esl_log(ESL_LOG_INFO, "clear and free all node and all value in the play_list\n"); 
	return 0;
}

//}}

//{{ 初始化及反初始化相关函数

/*
*@Description:init sip_config_t
*@param:status_cb:状态回调结构体
*@param:uuid:指定uuid字符串，若为NULL则程序会自动生成
*@param:phone_number:被叫号码
*@param:phome_prefix:被叫号码前缀
*@param:caller_id:主叫号码
*@param:domain:目的线路域名
*@param:codec:采用的sip编码，如 G722\G729\PCMU\PCMA, 默认为空，即协商解决
*@param:api_cmd:采用的呼叫命令，如&echo等， 默认未&park
*@param:record_path:录音路径
*@param:record_file_name:完整录音的录音文件名
*@return: success:sip_config_t faild:NULL
*/
sip_config_t sip_config_init( sip_status_cb_t status_cb,  char *uuid, char *phone_number, char *phone_prefix,  char *caller_id, char *domain, char *codec, char *api_cmd, char *record_path, char *record_file_name)
{
    //malloc config
    sip_config_t config = malloc(sizeof(sip_config));
    //init esl_handle
    config->handle = malloc(sizeof(esl_handle_t));
    memset(config->handle, 0, sizeof(esl_handle_t));
    config->play_handle = malloc(sizeof(esl_handle_t));
    memset(config->play_handle, 0, sizeof(esl_handle_t));
    //init status call back function
    config->status_cb = status_cb;
    //init sip_info
    config->info = (sip_info_t)malloc(sizeof(sip_info));
    config->info->domain = string_malloc_copy(domain);  
    config->info->codec = string_malloc_copy(codec);    
    config->info->phone_number = string_malloc_copy(phone_number);
    config->info->phone_prefix = string_malloc_copy(phone_prefix);
    config->info->caller_id = string_malloc_copy(caller_id);
    config->info->api_cmd = string_malloc_copy(api_cmd);
    config->info->record_path = string_malloc_copy(record_path);
    config->info->record_file_name = string_malloc_copy(record_file_name);
    //init uuid
    if(uuid != NULL){
        config->uuid = string_malloc_copy(uuid);
    }else{
        char uuid_tmp[32];
        generate_string(uuid_tmp, 30);
        config->uuid = string_malloc_copy(uuid_tmp);
        esl_log(ESL_LOG_INFO,"creat uuid[%s]\n",config->uuid);
    }
    //init play_list
    config->play_list = new_link_list();
    //log level init for test
    esl_global_set_default_logger(ESL_LOG_LEVEL_INFO);
    //esl connect
    esl_status_t status = esl_connect(config->handle, "127.0.0.1", 8021, NULL, "ClueCon");
    if (status != ESL_SUCCESS) {
        esl_log(ESL_LOG_INFO, "Connect Error: %d\n", status);
        return NULL;
    }else{
        esl_log(ESL_LOG_INFO, "Connected to FreeSWITCH\n");
    }
    status = esl_connect(config->play_handle, "127.0.0.1", 8021, NULL, "ClueCon");
    if (status != ESL_SUCCESS) {
        esl_log(ESL_LOG_INFO, "play handle connect Error: %d\n", status);
        return NULL;
    }else{
        esl_log(ESL_LOG_INFO, "play handle connected to FreeSWITCH\n");
    }
    esl_filter(config->handle, "unique-id", config->uuid);
    esl_events(config->handle, ESL_EVENT_TYPE_PLAIN,  "ALL");
    esl_log(ESL_LOG_INFO, "%s\n", config->handle->last_sr_reply);

	//init cb
	if(config->status_cb->init_cb != NULL)
	{
		config->status_cb->init_cb();
	}
	
    return config;
}
/*
*Description:free sip_info and member of it
*@param:info- the sip_info what we want to free
*@return: seccess: 0
*/
static inline int free_sip_info(sip_info_t info)
{
	SAFE_FREE(info->domain);
	SAFE_FREE(info->codec);
	SAFE_FREE(info->phone_number);
	SAFE_FREE(info->phone_prefix);
	SAFE_FREE(info->caller_id);	
	SAFE_FREE(info->api_cmd);
	SAFE_FREE(info->record_path);
	SAFE_FREE(info->record_file_name);
	SAFE_FREE(info);
	return 0;
}

int sip_config_uninit(sip_config_t config)
{
	//disconnect handle
	esl_disconnect(config->handle);
	//disconnect play_handle
	esl_disconnect(config->play_handle);
	//free handle
	SAFE_FREE(config->handle);
	//free play_handle
	SAFE_FREE(config->play_handle);
	
	//free info
	free_sip_info(config->info);
	//free uuid
	SAFE_FREE(config->uuid);
	//free play_list
	free_link_list_and_value(config->play_list);

	//free config
	SAFE_FREE(config);

	// unint cb
	if(config->status_cb->uninit_cb != NULL)
	{
		config->status_cb->uninit_cb();
	}
    return 0;
}


//}}

//{{ 线程相关，包括播放线程 和 事件监听及回调线程

void process_event(sip_config_t config, int *runflag, pthread_t *pid_play)
{
    esl_handle_t *handle = config->handle;
    char *uuid = config->uuid;
    esl_event_t *event = config->handle->last_ievent;
    char *this_uuid = esl_event_get_header(event, "Caller-Unique-ID");
    char *play_file_name = NULL;
    //esl_log(ESL_LOG_INFO, "[%s]%s\n", this_uuid, esl_event_name(event->event_id));
    /*
    */
    if(strcmp(uuid, this_uuid) == 0)
    {
        //esl_log(ESL_LOG_INFO, "[%s]%s\n", this_uuid, esl_event_name(event->event_id));
        switch (event->event_id) {
            case ESL_EVENT_CHANNEL_PARK:
            {
                esl_log(ESL_LOG_INFO, "channel parked~~~~\n");
                break;
            }
			case ESL_EVENT_CUSTOM:
			{
				
				break;
			}
            case ESL_EVENT_CHANNEL_EXECUTE:
            {    	
                break;
            }
            case ESL_EVENT_CHANNEL_EXECUTE_COMPLETE:
            {      	
                break;
            }
            case ESL_EVENT_CHANNEL_HANGUP_COMPLETE:
           {
                *runflag = 0;
                break;
           }
            case ESL_EVENT_CHANNEL_ANSWER:
           {
                gettimeofday(&(config->answer_time), NULL);
                esl_log(ESL_LOG_INFO, "channel answered~~~~\n");
                record_call(config); 
				if(config->status_cb->answer_cb != NULL)
				{
					config->status_cb->answer_cb();
				}				

                pthread_create(pid_play, NULL, (void *)&play_thread, config);  
				pthread_detach(*pid_play);
              
                break;
           }
            case ESL_EVENT_PLAYBACK_START:
           {
                play_file_name = esl_event_get_header(event, "variable_current_application_data");
                esl_log(ESL_LOG_INFO, "play [%s] start~ ~ ~\n", play_file_name);

                //修改playing_file_status状态标记{{    
                set_playing_file_status(play_file_name, config);
                //}}

				if(config->status_cb->playback_start_cb != NULL)
				{
					config->status_cb->playback_start_cb();
				}				

                //break_playing_file(config, 7000);//test break
                break;
           }
            case ESL_EVENT_PLAYBACK_STOP:
           {
                reset_playing_file_status(config);
                play_file_name = esl_event_get_header(event, "variable_current_application_data");
                esl_log(ESL_LOG_INFO, "play [%s] end~ ~ ~\n", play_file_name);
				
				if(config->status_cb->playback_end_cb != NULL)
				{
					config->status_cb->playback_end_cb();
				}				
                break;
           }
            case ESL_EVENT_RECORD_START:
           {
           		if(config->status_cb->record_start_cb != NULL)
				{
					config->status_cb->record_start_cb();
				}
                break;
           }
			case ESL_EVENT_DTMF:
			{
				if(config->status_cb->dtmf_cb != NULL)
				{
					config->status_cb->dtmf_cb();
				}
				break;
			}
            case ESL_EVENT_RECORD_STOP:
           {
           		if(config->status_cb->record_end_cb != NULL)
				{
					config->status_cb->record_end_cb();
				}
                break;
           }              
            case ESL_EVENT_CHANNEL_STATE:
           {
                char *channel_status = esl_event_get_header(event, "Channel-Call-State");
                esl_log(ESL_LOG_INFO, "stauts:[%s] ~~ ",channel_status);
                break;
           }   

            default:

                break;
        }
    }
}

void* event_listen_thread(void *arg)
{
    esl_status_t status;
    pthread_t pid_play = {0};
    sip_config_t config = (sip_config_t)arg;
    int runflag = 1;
    
    while(runflag && (status = esl_recv_event(config->handle, 1, NULL)) == ESL_SUCCESS) {
             
        if (config->handle->last_ievent) {
            process_event(config, &runflag, &pid_play);
        }
	}
    esl_log(ESL_LOG_INFO, "event listen thread ended ~ ~\n");
    return 0;
}

void* play_thread(void *arg)
{
    sip_config_t config = (sip_config_t)arg;
    esl_log(ESL_LOG_INFO, "play thread start ~ ~\n");
    struct timeval now;
    while(1)
    {
        gettimeofday(&now, NULL);
        if(timeval_sub(now, config->answer_time) < 300 ){
            usleep(5000);
        }else{
            break;
        }
    }
    while(1)
    {
        play_form_list(config);
        usleep(5000);
    }
    
    esl_log(ESL_LOG_INFO, "play thread ended ~ ~\n");
    return 0;
}

//}}
#if TEST
int main(void)
{
	
    /*
    esl_events(&handle, ESL_EVENT_TYPE_PLAIN,
        "SESSION_HEARTBEAT CHANNEL_ANSWER CHANNEL_ORIGINATE CHANNEL_PROGRESS CHANNEL_HANGUP "
			   "CHANNEL_BRIDGE CHANNEL_UNBRIDGE CHANNEL_OUTGOING CHANNEL_EXECUTE CHANNEL_EXECUTE_COMPLETE DTMF ");
    */
   
    
    //sip_config_t config = sip_config_init( NULL,  NULL, "1004", NULL, "123", NULL, NULL, NULL, "/root/record_path", "test1.wav");
    sip_config_t config = sip_config_init( NULL,  NULL, "13053075601", NULL, "123", "192.168.11.81", NULL, NULL, NULL, NULL);
    char play_file[512];
    struct timeval now;
    int answer_time_flag = 0;

    pthread_t pid_listen = {0};

    make_call(config);

    config->handle->event_lock = 1;
  
    add_to_playlist("/opt/swmy.wav", config, 1, 0);
    add_silence_to_playlist(5000, config, 1, 0);
    add_to_playlist("/opt/swmy.wav", config, 1, 0);

    pthread_create(&pid_listen, NULL, (void *)&event_listen_thread, config);

    /*创建语音文件播放线程*/
    /*
    
    if (0 != pthread_create(&pid_play, NULL, (void *)&play_thread, config)){
        // log_error("create play thread error:%s(errno:%d)", strerror(errno), errno);
        // yfs_media_thread_quit();
        // pthread_join(g_pid_capture, NULL);
        // SAFE_FERR_ASR_HANDLE(asr_handle);
        // event_queue_uninit(&media_event_queu);
        return -1;
    }
    */
    pthread_join(pid_listen, NULL);


end:
	sip_config_uninit(config);
	//esl_disconnect(config->handle);
    /*
    */
	return 0;
}
#endif
