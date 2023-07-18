#include "com_test.h"
#include "user_interface.h"
#include "record.h"

extern int angle_int;
extern int if_awake;

init_rec = 0;
init_success = 0;
write_first_data = 0;

int once_awake;
int wake_switch = 1;

unsigned char* record_data;

//音频数据处理
int business_data(unsigned char* record)
{
    record_data = record;
    if (!init_success && init_rec)
    {
        int len = 3*PCM_MSG_LEN;
        char *pcm_buffer=(char *)malloc(len);
        if (NULL == pcm_buffer)
            {
                printf(">>>>>buffer is null\n");
                exit (1);
            }
        memcpy(pcm_buffer, record_data, len);

        if (write_first_data++ == 0)
        {
    #if whether_print_log
            printf("***************write the first voice**********\n");
    #endif
            demo_xf_mic(pcm_buffer, len, 1);
        }

        else
        {
    #if whether_print_log
            printf("***************write the middle voice**********\n");
    #endif
            demo_xf_mic(pcm_buffer, len, 2);
        }
        if (whether_finised)
        {
            record_finish = 1;
            whether_finised = 0;
        }
    }   
}

//麦克风串口启动唤醒
void *com_wakeup(void* args)
{
    int fd=1, read_num = 0;
    unsigned char buffer[1];
    memset(buffer, 0, 1);
    char* uartname="/dev/tarkbot_mic";
    if((fd=open_port(uartname))<0)
    {
         printf("open %s is failed\n",uartname);
         return 0;
    }
    else{
            set_opt(fd, 115200, 8, 'N', 1);
            //printf("set_opt fd=%d\n",fd);
            while(1){
                    usleep(1000);
                    memset(buffer, 0, 1);
                    read_num = read(fd, buffer, 1);
                    //printf("read_num=%d\n",read_num);
                    if(read_num>0){
						//printf("%x\n", buffer[0]);
						deal_with(buffer[0]);
                    }
                    //else{
                    //    printf("read error\n");
                    //}
                    if(if_awake){
						//printf("angle : [%d]\n",angle_int);
                        sleep(0.8);
						if_awake = 0;
                        wake_switch = 0;
					} 
            }
        fd=close(fd);
    }
    return 0;
}

//识别计时
void *time_count(void* args)
{
    while(1){

            if (!init_success && init_rec)
            {
                //获取当前时间
                clock_t start, finish;
                double total_time;
                start = clock();
                while (!init_success && whether_finised != 1)
                {   
                    finish = clock();
                    total_time = (double)(finish - start) / CLOCKS_PER_SEC/2;
                    //printf(">>>>>total_time:　[ %f ]\n", total_time);
                    //printf(">>>>>whether_finised:　[ %d ]\n", whether_finised);
                    if (total_time > max_asr_time)
                    {
                        printf(">>>>>超出离线命令词最长识别时间\n");
                        record_finish = 1; 
                        break;
                    }
                } 
            }

            
    }
    return 0;
}

/********************获取识别到的内容里关键词、置信度以及关键词的id******************/
Effective_Result show_result(const char *string, const int input_confidence)
{
    Effective_Result current;
    int confidence_int = 0;
    int order_id = 0;

    if (strlen(string) > 250)
    {
        char asr_result[32];    //识别到的关键字的结果
        char asr_confidence[3]; //识别到的关键字的置信度
        char str_orderID[8];    //识别到的关键字的id

        char *p1 = strstr(string, "<rawtext>");
        char *p2 = strstr(string, "</rawtext>");
        int n1 = p1 - string + 1;
        int n2 = p2 - string + 1;

        char *p3 = strstr(string, "<confidence>");
        char *p4 = strstr(string, "</confidence>");
        int n3 = p3 - string + 1;
        int n4 = p4 - string + 1;
        for (int i = 0; i < 32; i++)
        {
            asr_result[i] = '\0';
        }

        strncpy(asr_confidence, string + n3 + strlen("<confidence>") - 1, n4 - n3 - strlen("<confidence>"));
        asr_confidence[n4 - n3 - strlen("<confidence>")] = '\0';
        confidence_int = atoi(asr_confidence);

        if (confidence_int >= input_confidence)
        {
            strncpy(asr_result, string + n1 + strlen("<rawtext>") - 1, n2 - n1 - strlen("<rawtext>"));
            asr_result[n2 - n1 - strlen("<rawtext>")] = '\0'; //加上字符串结束符。

            memset(str_orderID, 0, sizeof(str_orderID));
            char *str_todo = strstr(p3, "id=");
            char *str_todo_back = strstr(str_todo, ">");
            strncpy(str_orderID, str_todo + 4, str_todo_back - str_todo - 5);
            order_id = atoi(str_orderID);
        }
        else
        {
            strncpy(asr_result, "", 0);
            order_id = 0;
        }

        current.effective_confidence = confidence_int;
        strcpy(current.effective_word, asr_result);
        current.effective_id = order_id;
        return current;
    }
    else
    {
        current.effective_confidence = 0;
        strcpy(current.effective_word, " ");
        current.effective_id = 0;
        return current;
    }
}
 
int main()
{
    int ret = 0;
    pthread_t pid1;
    pthread_t pid2;
    if(pthread_create(&pid1, NULL, com_wakeup, NULL))
    {
        return -1;
    }

    if(pthread_create(&pid2, NULL, time_count, NULL))
    {
        return -1;
    }

    Recognise_Result inital = initial_asr_paramers(ASR_RES_PATH, GRM_BUILD_PATH, GRM_FILE, LEX_NAME);  //语法相关参数初始化
 
    while(1){

        if (!if_awake && wake_switch)
        {
            printf(">>>>>待唤醒，请用唤醒词进行唤醒！\n");
        }
        while(!if_awake && wake_switch)
        {
            sleep(1);
        }
        printf("\n>>>>>唤醒成功，已开启录音！\n");
        printf("angle : [%d]\n",angle_int);
        once_awake = awake_count;
        whether_finised = 0;
        wake_switch = 1;
        
        while(once_awake)
        {
            once_awake--;
            record_finish = 0;
            const char file1[] = DENOISE_SOUND_PATH;  //讲话内容转为音频
            printf(">>>>>正在识别...\n");

            ret = create_asr_engine(&asr_data);  //创建离线识别引擎
            if (MSP_SUCCESS != ret)
            {
#if whether_print_log
                printf("[01]创建语音识别引擎失败！\n");
#endif
            }

            get_the_record_sound(file1);  //音频识别处理

            if (whole_result!="")  //识别到的内容
            {
                
                /**为了在终端可视化方便,可将本行注释，若需查看完整反馈，可解除屏蔽**/
                //printf(">>>>>全部返回结果:　[ %s ]\n", result.whole_content);
                Effective_Result effective_ans = show_result(whole_result, confidence); //
                if (effective_ans.effective_confidence >= confidence) //如果大于置信度阈值则进行显示或者其他控制操作
                {
                    printf(">>>>>是否识别成功:　[ %s ]\n", "是");
                    printf(">>>>>关键字的置信度: [ %d ]\n", effective_ans.effective_confidence);
                    printf(">>>>>关键字识别结果: [ %s ]\n", effective_ans.effective_word);
                }
                else
                {
                    printf(">>>>>是否识别成功:　[ %s ]\n", "否");
                    printf(">>>>>关键字的置信度: [ %d ]\n", effective_ans.effective_confidence);
                    printf(">>>>>关键字置信度较低，文本不予显示\n");
                }
            }
            else
            {
                printf(">>>>>未能检测到有效声音,请重试\n");
            }
            if (!wake_switch)
            {
                break;
            }
    delete_asr_engine();  //关闭离线识别引擎
    whole_result = "";   //初始化
    write_first_data = 0;
    sleep(1);
    }
 }
    return 0;
}