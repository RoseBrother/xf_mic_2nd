/* user_interface_h */
#include "asr_offline_record_sample.h"
/***************************参数配置区域，用户可通过修改这些词来进行 ***************************************/
#define whether_print_log 0 //是否打印log

int PCM_MSG_LEN = 1024; //在录音时会发布音频流,单次发布大小为2048B

/********************************[1]　运行效果调试参数****************************************/
int confidence = 40; //置信度阈值，可根据麦克风使用环境进行设置，若容易检测出错，则调大该值。
int max_asr_time = 10;
int time_per_order = 15;//一次录音时长
int awake_count = 5;//一次唤醒，可允许对话的次数
char *LEX_NAME = (char*)"contact";
char *APPID = "37864a4a";//"5e159d11"; //APPID
/*************************[2]语法识别资源路径、语法路径、音频文件保存地址等**********************/
//语法相关参数,路径最好设置为绝对地址
char *ASR_RES_PATH = (char*)"fo|/home/xtark/tarkbot_mic/M2SDK_OLD/SR2.2-HR1.1.3_line/offline_mic/bin/config/msc/res/asr/common.jet"; //离线语法识别资源路径，重要，与麦克风及appid绑定
char *GRM_BUILD_PATH = (char*)"/home/xtark/tarkbot_mic/M2SDK_OLD/SR2.2-HR1.1.3_line/offline_mic/bin/config/msc/res/asr/GrmBuilld";   //构建离线语法识别网络生成数据保存路径
char *GRM_FILE = (char*)"/home/xtark/tarkbot_mic/M2SDK_OLD/SR2.2-HR1.1.3_line/offline_mic/bin/call.bnf";					//构建离线识别语法网络所用的语法文件，用户自修改文件
//录音文件保存的地址,最好设置为绝对地址 
#define DENOISE_SOUND_PATH "../audio/vvui_deno.pcm"

UserData asr_data;
int whether_finised;


