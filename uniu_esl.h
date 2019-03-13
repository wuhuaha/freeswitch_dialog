#ifndef __UNIU_ESL_H__
#define __UNIU_ESL_H__
#include <esl.h>
#include "uniu_list.h"
#include <time.h>
#include <sys/time.h>

#ifndef SAFE_FREE
#define SAFE_FREE(ptr) \
{ \
    if (NULL != ptr) { \
        free(ptr); \
        ptr = NULL; \
    } \
}
#endif
//回调函数声明
typedef struct
{
    int (*init_cb)(void*); //初始化
    int (*calling_cb)(void*);//呼叫
    int (*incoming_cb)(void*);//收到呼入
    int (*early_cb)(void*);//对方收到呼叫
    int (*confirmed_cb)(void*);//正在通话
    int (*disconnetd_cb)(void*);//挂断
    int (*disconfirmed_cb)(void*);//不接听主动挂断
    int (*dtmp_cb)(void*);//按键
    int (*playback_start_cb)(void*);//开始播放音频
    int (*playback_end_cb)(void*);//播放音频结束
} sip_status_cb;

typedef sip_status_cb * sip_status_cb_t;

//呼叫信息
typedef struct
{
    char *phone_number;//被叫号码 如 10000
    char *phone_prefix;//前缀   如944，若没有可为NULL
    char *caller_id;//主叫号码  如18001024001
    char *domain;//sip的域名或ip及端口号 如192.168.1.23：5060，若没有可为NULL，默认为本机注册号码
    //注：可选项： [G722 , OPUS, PCMU,  PCMA,  G729]；
    char *codec;//采用的编码  如"G722"
    char *api_cmd;//呼叫后执行的api 用法1：默认为&park,然后可以调用client函数监听并操作； 用法2：指定目的server，然后在server端处理后续通话
    char *record_path;
    char *record_file_name;
} sip_info;

typedef sip_info * sip_info_t;

typedef struct
{
    char *domain;//fs的域名
    char *port;//esl端口号
    char *usr;//esl密码
    char *passwd;//esl密码
} esl_info;

typedef esl_info * esl_info_t;

typedef struct
{
   char play_file_name[1024];
   struct timeval play_start_time; 
   double play_file_length_ms;
} play_status;

typedef play_status * play_status_t;

typedef enum
{
    CUSTOM_ACTIVE,
    CUSTOM_INACTIVE
} active_stauts;

typedef struct
{
    esl_handle_t *handle;//esl handle
    esl_info_t esl;//esl配置信息
    sip_status_cb_t status_cb;//状态回调
    sip_info_t info; //sip呼叫信息
    char *uuid; //本通电话的特征识别码
    link_t play_list;//播放链表
    play_status playing_file_status;
    active_stauts if_active;
    struct timeval answer_time;
} sip_config;

typedef sip_config * sip_config_t;

//将音频文件加入到播放列表中
int add_to_playlist(char *file_name, sip_config_t config, int if_last, int if_clean);

// 获取wav文件的时长，单位：毫秒
double get_wav_time_length(char* file_name);

int play_wav_file(char *file_name,  sip_config_t config);

int make_call(sip_config_t config);

sip_config_t sip_config_init( sip_status_cb_t status_cb,  char *uuid, char *phone_number, char *phone_prefix,  char *caller_id, char *domain, char *codec, char *api_cmd, char *record_path, char *record_file_name);

//when playing file last time less than protect(ms), clean play list, else, break it and clean play list
int break_playing_file(sip_config_t config, int protect);

//将正在拨打的音频重置掉
int reset_playing_file_status(sip_config_t config);

char *get_from_playlist(sip_config_t config, char *play_file_name);

#endif