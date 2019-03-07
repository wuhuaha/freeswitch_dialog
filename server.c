#include <stdio.h>
#include <stdlib.h>
#include <esl.h>
#include <time.h>

#define ERROR_PROMPT "say:输入错误，请重新输入"

char *generate_string(char *str, int length)
{
    register int i,flag;
     
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
    esl_log(ESL_LOG_INFO,"%s\n", str);//输出生成的随机数。
     
    return str;
}


int check_account_password(const char *account, const char *password)
{
    return (!strcmp(account, "1111")) && (!strcmp(password, "1111"));
}

void process_event(esl_handle_t *handle, esl_event_t *event)
{
    const char *uuid = esl_event_get_header(event, "Caller-Unique-ID");

    uuid = strdup(uuid);
    if (!uuid) abort();

    switch (event->event_id) {
        case ESL_EVENT_CHANNEL_PARK:
        {
            const char *service;

            service = esl_event_get_header(event, "variable_service");
            esl_log(ESL_LOG_INFO, "Service: %s\n", service);

            if (!service || (service && strcmp(service, "icharge"))) break;

            esl_log(ESL_LOG_INFO, "New Call %s\n", uuid);

            esl_execute(handle, "answer", NULL, uuid);
			esl_execute(&handle, "playback", "/opt/swmy.wav", NULL);
			sleep(2);
			esl_execute(&handle, "hangup", NULL, NULL);
			/*
            esl_execute(handle, "set", "tts_engine=tts_commandline", uuid);
            esl_execute(handle, "set", "tts_voice=Ting-Ting", uuid);
            esl_execute(handle, "speak", "您好，欢迎使用空中充值服务", uuid);

again:
            esl_execute(handle, "set", "charge_state=WAIT_ACCOUNT", uuid);

            esl_execute(handle, "play_and_get_digits",
                "4 5 3 5000 # 'say:请输入您的账号，以井号结束' "
                ERROR_PROMPT " charge_account ^\\d{4}$", uuid);

            esl_execute(handle, "set", "charge_state=WAIT_PASSWORD", uuid);

            esl_execute(handle, "play_and_get_digits",
                "4 5 3 5000 # 'say:请输入您的密码，以井号结束' "
                ERROR_PROMPT " charge_password ^\\d{4}$", uuid);
			*/
            break;
        }
        case ESL_EVENT_CHANNEL_EXECUTE_COMPLETE:
        {
            const char *application;
            const char *charge_state;

            application = esl_event_get_header(event, "Application");
            charge_state = esl_event_get_header(event, "variable_charge_state");

            if (!strcmp(application, "play_and_get_digits") &&
                !strcmp(charge_state, "WAIT_PASSWORD")) {

                const char *account = esl_event_get_header(event, "variable_charge_account");
                const char *password = esl_event_get_header(event, "variable_charge_password");

                if (account && password && check_account_password(account, password)) {
                    esl_log(ESL_LOG_INFO, "Account: %s Balance: 100\n", account);
                    esl_execute(handle, "speak", "您的余额是100元", uuid);
                    esl_execute(handle, "speak", "再见", uuid);
                    esl_execute(handle, "hangup", NULL, uuid);
                } else {
                    esl_execute(handle, "speak", "账号密码错误", uuid);
                    goto again;
                }
            }
            break;
        }
        case ESL_EVENT_CHANNEL_HANGUP_COMPLETE:
            esl_log(ESL_LOG_INFO, "Hangup %s\n", uuid);
            break;
        default:
			esl_log(ESL_LOG_INFO, "[%s]%s\n", uuid, esl_event_name(event->event_id));
            break;
    }

    free((void *)uuid);
}

int make_call(esl_handle_t *handle, char *phone_number, char *phone_prefix, char *uuid, char *caller_id, char *domain, char *codec, char *api_cmd)
{
    char call_string[1024];
    char sip_string[128];  //example: sip:1001@192.168.1.1:5060  or  user/1001
    char uuid_string[64];
    char uuid_generate[64];
    char codec_string[64];
	char api_string[64];
    char caller_id_string[128];

	//标准化sip_string为标准格式，在有domain时使用domain呼叫，否则认为是本机注册用户
    if(domain != NULL){
        snprintf(sip_string, sizeof(sip_string), "sip:%s%s@%s", (phone_prefix == NULL ? "" : phone_prefix), phone_number, domain);
    }else{
        snprintf(sip_string, sizeof(sip_string), "user/%s%s", (phone_prefix == NULL ? "" : phone_prefix), phone_number);
    }

    if(codec != NULL){
        snprintf(codec_string, sizeof(codec_string), "{absolute_codec_string=%s}", codec);    
    }else{
        *codec_string = 0;
    }

    if(uuid != NULL){
        snprintf(uuid_string, sizeof(uuid_string), "{origination_uuid=%s}", uuid);
    }else{
        generate_string(uuid_generate, 10);
        snprintf(uuid_string, sizeof(uuid_string), "{origination_uuid=%s}", uuid_generate);
    }

    if(caller_id != NULL){
        snprintf(caller_id_string, sizeof(caller_id_string), "{origination_caller_id_number=%s}{origination_caller_id_name=%s}", caller_id, caller_id);    
    }else{
        *caller_id_string = 0;
    }

	if(api_cmd != NULL){
        snprintf(api_string, sizeof(api_string), "%s", api_cmd);    
    }else{
        snprintf(api_string, sizeof(api_string), "&park");;
    }

    snprintf(call_string, sizeof(call_string), "bgapi originate %s%s%s%s %s", uuid_string, caller_id_string, codec_string, sip_string, api_string);
    esl_log(ESL_LOG_INFO,"%s\n", call_string);
    esl_send(handle, call_string);
}

int main(void)
{
	esl_handle_t handle = {{0}};
	esl_status_t status;
    const char *uuid;

    esl_global_set_default_logger(ESL_LOG_LEVEL_INFO);

    status = esl_connect(&handle, "127.0.0.1", 8021, NULL, "ClueCon");

    if (status != ESL_SUCCESS) {
        esl_log(ESL_LOG_INFO, "Connect Error: %d\n", status);
        exit(1);
    }

    esl_log(ESL_LOG_INFO, "Connected to FreeSWITCH\n");
    esl_events(&handle, ESL_EVENT_TYPE_PLAIN,
        "SESSION_HEARTBEAT CHANNEL_ANSWER CHANNEL_ORIGINATE CHANNEL_PROGRESS CHANNEL_HANGUP "
			   "CHANNEL_BRIDGE CHANNEL_UNBRIDGE CHANNEL_OUTGOING CHANNEL_EXECUTE CHANNEL_EXECUTE_COMPLETE DTMF");
    esl_log(ESL_LOG_INFO, "%s\n", handle.last_sr_reply);

	//esl_send(&handle, "api originate {origination_uuid=123456789}user/1004 &echo");
    make_call(&handle, "1004", NULL, NULL, "123", NULL, NULL, NULL);

    handle.event_lock = 1;
    while((status = esl_recv_event(&handle, 1, NULL)) == ESL_SUCCESS) {
        if (handle.last_ievent) {
            process_event(&handle, handle.last_ievent);
        }
	}

end:

	esl_disconnect(&handle);

	return 0;
}