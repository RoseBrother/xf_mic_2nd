#include "user_interface.h"

static char _ASR_RES_PATH[120] = " ";   //离线语法识别资源路径，重要，与麦克风及appid绑定
static char _GRM_BUILD_PATH[120] = " "; //构建离线语法识别网络生成数据保存路径
static char _GRM_FILE[120] = " ";	   //构建离线识别语法网络所用的语法文件，用户自修改文件
static char _LEX_NAME[20] = " ";

struct speech_rec iat;

int build_grm_cb(int ecode, const char *info, void *udata)
{
	UserData *grm_data = (UserData *)udata;

	if (NULL != grm_data) {
		grm_data->build_fini = 1;
		grm_data->errcode = ecode;
	}

	if (MSP_SUCCESS == ecode && NULL != info) {
		//printf("构建语法成功！ 语法ID:%s\n", info);
		if (NULL != grm_data)
			snprintf(grm_data->grammar_id, MAX_GRAMMARID_LEN - 1, info);
	}
	else
		printf("构建语法失败！%d\n", ecode);

	return 0;
}

int build_grammar(UserData *udata)
{
	FILE *grm_file                           = NULL;
	char *grm_content                        = NULL;
	unsigned int grm_cnt_len                 = 0;
	char grm_build_params[MAX_PARAMS_LEN]    = {NULL};
	int ret                                  = 0;

	grm_file = fopen(_GRM_FILE, "rb");	
	if(NULL == grm_file) {
		printf("打开\"%s\"文件失败！[%s]\n", _GRM_FILE, strerror(errno));
		return -1; 
	}

	fseek(grm_file, 0, SEEK_END);
	grm_cnt_len = ftell(grm_file);
	fseek(grm_file, 0, SEEK_SET);

	grm_content = (char *)malloc(grm_cnt_len + 1);
	if (NULL == grm_content)
	{
		printf("内存分配失败!\n");
		fclose(grm_file);
		grm_file = NULL;
		return -1;
	}
	fread((void*)grm_content, 1, grm_cnt_len, grm_file);
	grm_content[grm_cnt_len] = '\0';
	fclose(grm_file);
	grm_file = NULL;

	snprintf(grm_build_params, MAX_PARAMS_LEN - 1, 
		"engine_type = local, \
		asr_res_path = %s, sample_rate = %d, \
		grm_build_path = %s, ",
		_ASR_RES_PATH,
		SAMPLE_RATE_16K,
		_GRM_BUILD_PATH
		);
	ret = QISRBuildGrammar("bnf", grm_content, grm_cnt_len, grm_build_params, build_grm_cb, udata);

	free(grm_content);
	grm_content = NULL;

	return ret;
}

int update_lex_cb(int ecode, const char *info, void *udata)
{
	UserData *lex_data = (UserData *)udata;

	if (NULL != lex_data) {
		lex_data->update_fini = 1;
		lex_data->errcode = ecode;
	}

	if (MSP_SUCCESS == ecode)
		printf("更新词典成功！\n");
	else
		printf("更新词典失败！%d\n", ecode);

	return 0;
}

int update_lexicon(UserData *udata)
{
	const char *lex_content                   = "丁伟\n黄辣椒";
	unsigned int lex_cnt_len                  = strlen(lex_content);
	char update_lex_params[MAX_PARAMS_LEN]    = {NULL}; 

	snprintf(update_lex_params, MAX_PARAMS_LEN - 1, 
		"engine_type = local, text_encoding = UTF-8, \
		asr_res_path = %s, sample_rate = %d, \
		grm_build_path = %s, grammar_list = %s, ",
		_ASR_RES_PATH,
		SAMPLE_RATE_16K,
		_GRM_BUILD_PATH,
		udata->grammar_id);
	return QISRUpdateLexicon(_LEX_NAME, lex_content, lex_cnt_len, update_lex_params, update_lex_cb, udata);
}


static void show_result(char *string, char is_over)
{
	printf("\rResult: [ %s ]", string);
	if(is_over)
		putchar('\n');
}

static char *g_result = NULL;
static unsigned int g_buffersize = BUFFER_SIZE;

void on_result(const char *result, char is_last)
{
	if (result) {
		size_t left = g_buffersize - 1 - strlen(g_result);
		size_t size = strlen(result);
		if (left < size) {
			g_result = (char*)realloc(g_result, g_buffersize + BUFFER_SIZE);
			if (g_result)
				g_buffersize += BUFFER_SIZE;
			else {
				printf("mem alloc failed\n");
				return;
			}
		}
		strncat(g_result, result, size);
		//show_result(g_result, is_last);
		whole_result = g_result;
	}
	else
	{
		strncpy(whole_result, "", 0);
	}
}
void on_speech_begin()
{
	if (g_result)
	{
		free(g_result);
	}
	g_result = (char*)malloc(BUFFER_SIZE);
	g_buffersize = BUFFER_SIZE;
	memset(g_result, 0, g_buffersize);

	//printf("Start Listening...\n");
}
void on_speech_end(int reason)
{
	if (reason == END_REASON_VAD_DETECT)
		//printf("\nSpeaking done \n");
		printf("\n");
		
	else
		printf("\nRecognizer error %d\n", reason);
}

/* demo send audio data from a file */
static void demo_file(const char* audio_file, const char* session_begin_params)
{
	int	errcode = 0;
	FILE*	f_pcm = NULL;
	char*	p_pcm = NULL;
	unsigned long	pcm_count = 0;
	unsigned long	pcm_size = 0;
	unsigned long	read_size = 0;
	struct speech_rec iat;
	struct speech_rec_notifier recnotifier = {
		on_result,
		on_speech_begin,
		on_speech_end
	};

	if (NULL == audio_file)
		goto iat_exit;

	f_pcm = fopen(audio_file, "rb");
	if (NULL == f_pcm)
	{
		printf("\nopen [%s] failed! \n", audio_file);
		goto iat_exit;
	}

	fseek(f_pcm, 0, SEEK_END);
	pcm_size = ftell(f_pcm);
	fseek(f_pcm, 0, SEEK_SET);

	p_pcm = (char *)malloc(pcm_size);
	if (NULL == p_pcm)
	{
		printf("\nout of memory! \n");
		goto iat_exit;
	}

	read_size = fread((void *)p_pcm, 1, pcm_size, f_pcm);
	if (read_size != pcm_size)
	{
		printf("\nread [%s] error!\n", audio_file);
		goto iat_exit;
	}

	errcode = sr_init(&iat, session_begin_params, SR_USER, &recnotifier);
	if (errcode) {
		printf("speech recognizer init failed : %d\n", errcode);
		goto iat_exit;
	}

	errcode = sr_start_listening(&iat);
	if (errcode) {
		printf("\nsr_start_listening failed! error code:%d\n", errcode);
		goto iat_exit;
	}

	while (1)
	{
		unsigned int len = 10 * FRAME_LEN; /* 200ms audio */
		int ret = 0;

		if (pcm_size < 2 * len)
			len = pcm_size;
		if (len <= 0)
			break;

		ret = sr_write_audio_data(&iat, &p_pcm[pcm_count], len);

		if (0 != ret)
		{
			printf("\nwrite audio data failed! error code:%d\n", ret);
			goto iat_exit;
		}

		pcm_count += (long)len;
		pcm_size -= (long)len;		
	}

	errcode = sr_stop_listening(&iat);
	if (errcode) {
		printf("\nsr_stop_listening failed! error code:%d \n", errcode);
		goto iat_exit;
	}

iat_exit:
	if (NULL != f_pcm)
	{
		fclose(f_pcm);
		f_pcm = NULL;
	}
	if (NULL != p_pcm)
	{
		free(p_pcm);
		p_pcm = NULL;
	}

	sr_stop_listening(&iat);
	sr_uninit(&iat);
}

/* demo recognize the audio from microphone */
void demo_xf_mic(char *waveData,unsigned int waveLen,int audio_status)
{
	int errcode;
	int ret = 0;

	iat.audio_status = audio_status;
#if whether_print_log
	printf("states is :%d\n", iat.audio_status);
#endif

	if (iat.ep_stat != 3)
	{
		//printf("writing..............\n");
		ret = sr_write_audio_data(&iat, waveData, waveLen); //写入数据块
	}
	else
	{
#if whether_print_log
		printf("detect the end of talk\n");
#endif
		whether_finised = 1; // 不在写入
	}
	if (0 != ret)
	{
		printf("\nwrite audio data failed! error code:%d\n", ret);
		//goto iat_exit;
	}
/*iat_exit:
	sr_stop_listening(&iat);
	sr_uninit(&iat);*/

}

/*关闭离线识别引擎*/
void delete_asr_engine()
{
	int errcode = 0;
	errcode = sr_stop_listening(&iat);
	if (errcode)
	{
		printf("\nsr_stop_listening failed! error code:%d \n", errcode);
		goto iat_exit;
	}
	sr_uninit(&iat);
iat_exit:
	sr_stop_listening(&iat);
	sr_uninit(&iat);
}

/*创建离线识别引擎*/
int create_asr_engine(UserData *udata)
{
	char asr_params[MAX_PARAMS_LEN] = {NULL};
	const char *rec_rslt = NULL;
	const char *session_id = NULL;
	const char *asr_audiof = NULL;
	FILE *f_pcm = NULL;
	char *pcm_data = NULL;
	long pcm_count = 0;
	long pcm_size = 0;
	int last_audio = 0;

	int aud_stat = MSP_AUDIO_SAMPLE_CONTINUE;
	int ep_status = MSP_EP_LOOKING_FOR_SPEECH;
	int rec_status = MSP_REC_STATUS_INCOMPLETE;
	int rss_status = MSP_REC_STATUS_INCOMPLETE;
	int errcode = -1;
	int aud_src = 0;
	//const char *ASR_RES_PATH = asr_res_path; //－－－－－－－－－－－－－－－－－－－－－－－
	//const char *GRM_BUILD_PATH = grm_build_path;
	//离线语法识别参数设置
	snprintf(asr_params, MAX_PARAMS_LEN - 1,
			 "engine_type = local, \
		asr_res_path = %s, sample_rate = %d, \
		grm_build_path = %s, local_grammar = %s, \
		result_type = xml, result_encoding = UTF-8, \
		vad_eos = 900, asr_denoise = 1, vad_enable = 1, ",
			 _ASR_RES_PATH,
			 SAMPLE_RATE_16K,
			 _GRM_BUILD_PATH,
			 udata->grammar_id);

	struct speech_rec_notifier recnotifier = {
		on_result,
		on_speech_begin,
		on_speech_end};

	errcode = sr_init(&iat, asr_params, SR_USER, &recnotifier);
	if (errcode)
	{
		printf("speech recognizer init failed : %d\n", errcode);
		goto iat_exit;
	}

	errcode = sr_start_listening(&iat);
	if (errcode)
	{
		printf("\nsr_start_listening failed! error code:%d\n", errcode);
		goto iat_exit;
	}
	return 0;
iat_exit:
	sr_stop_listening(&iat);
	sr_uninit(&iat);
}

int run_asr(UserData *udata,unsigned char *file)
{
	char asr_params[MAX_PARAMS_LEN]    = {NULL};
	const char *rec_rslt               = NULL;
	const char *session_id             = NULL;
	const char *asr_audiof             = NULL;
	FILE *f_pcm                        = NULL;
	char *pcm_data                     = NULL;
	long pcm_count                     = 0;
	long pcm_size                      = 0;
	int last_audio                     = 0;

	int aud_stat                       = MSP_AUDIO_SAMPLE_CONTINUE;
	int ep_status                      = MSP_EP_LOOKING_FOR_SPEECH;
	int rec_status                     = MSP_REC_STATUS_INCOMPLETE;
	int rss_status                     = MSP_REC_STATUS_INCOMPLETE;
	int errcode                        = -1;
	int aud_src                        = 0;
	//离线语法识别参数设置
	snprintf(asr_params, MAX_PARAMS_LEN - 1, 
		"engine_type = local, \
		asr_res_path = %s, sample_rate = %d, \
		grm_build_path = %s, local_grammar = %s, \
		result_type = xml, result_encoding = UTF-8, ",
		_ASR_RES_PATH,
		SAMPLE_RATE_16K,
		_GRM_BUILD_PATH,
		udata->grammar_id
		);
/*	printf("音频数据在哪? \n0: 从文件读入\n1:从MIC说话\n");
	scanf("%d", &aud_src);
	if(aud_src != 0) {
		demo_mic(asr_params);
	} else {
		asr_audiof = get_audio_file();
		demo_file(asr_audiof, asr_params); 
	}*/
	demo_file(file, asr_params);
	return 0;
}

Recognise_Result initial_asr_paramers(char *jet_path, char *grammer_path, char *bnf_path, char *lex_na)
{
	strcpy(_ASR_RES_PATH, jet_path);		  //离线语法识别资源路径，重要，与麦克风及appid绑定
	strcpy(_GRM_BUILD_PATH, grammer_path); //构建离线语法识别网络生成数据保存路径
	strcpy(_GRM_FILE, bnf_path);			  //构建离线识别语法网络所用的语法文件，用户自修改文件
	strcpy(_LEX_NAME, lex_na);
	Recognise_Result result;
	int ret = 0;
	if (sizeof(APPID) < 8)
	{
#if whether_print_log
		printf("[log]APPID不符合要求：%d\n", ret);
#endif
		result.whether_recognised = false;
		strcpy(result.fail_reason, "appid_error");
		goto exit;
	}
#if whether_print_log
	printf("[log]APPID符合要求：%d\n", ret);
#endif
	char login_config[35];
	sprintf(login_config, "appid = %s, work_dir = .", APPID);

	//const char *login_params = "appid = 5de09283, work_dir = /home/nie/cream_ws/src/xf_mic_package/config/";
	/*[0].通用过程,用户注册,声明这次使用离线引擎或者在线引擎]*/
	ret = MSPLogin(NULL, NULL, login_config); //第一个参数为用户名，第二个参数为密码，传NULL即可，第三个参数是登录参数
	if (MSP_SUCCESS != ret)
	{
#if whether_print_log
		printf("[log]登录失败：%d\n", ret);
#endif
		result.whether_recognised = false;
		strcpy(result.fail_reason, "login error");
		goto exit;
	}
#if whether_print_log
	printf("[log]登录成功：%d\n", ret);
#endif
	memset(&asr_data, 0, sizeof(UserData));
#if whether_print_log
	printf("[log]构建离线识别语法网络...\n");
#endif

	/*[1].创建了语法,需要bnf文件,jet文件和要存放的grammer地址,做完这一步,grammer就创建完成了]*/
	ret = build_grammar(&asr_data); //第一次使用某语法进行识别，需要先构建语法网络，获取语法ID，之后使用此语法进行识别，无需再次构建
	if (MSP_SUCCESS != ret)
	{
#if whether_print_log
		printf("[log]构建语法调用失败！\n");
#endif
		result.whether_recognised = false;
		strcpy(result.fail_reason, "build_grammer_error");
		goto exit;
	}
#if whether_print_log
	printf("[1]构建语法成功！\n");
#endif

	result.whether_recognised = true;
	strcpy(result.fail_reason, "　");
	if (strlen(whole_result) > 0)
	{
		result.whole_content = whole_result;
	}
	else
	{
		result.whole_content = " ";
	}
	return result;
exit:
	MSPLogout();
	return result;
}

Recognise_Result deal_with_record(unsigned char *path, char *jet_path, char *grammer_path, char *bnf_path, char *lex_na)
{
	strcpy(_ASR_RES_PATH, jet_path); //离线语法识别资源路径，重要，与麦克风及appid绑定
	strcpy(_GRM_BUILD_PATH, grammer_path); //构建离线语法识别网络生成数据保存路径
	strcpy(_GRM_FILE, bnf_path); //构建离线识别语法网络所用的语法文件，用户自修改文件
	strcpy(_LEX_NAME, lex_na);
	int ret = 0;
	Recognise_Result result;
	if (sizeof(APPID)<8)
	{
#if whether_print_log
		printf("[log]APPID不符合要求：%d\n", ret);
#endif
		result.whether_recognised = false;
		strcpy(result.fail_reason, "appid_error");
		goto exit;
	}
#if whether_print_log
		printf("[log]APPID符合要求：%d\n", ret);
#endif
	char login_config[35];
	sprintf(login_config, "appid = %s, work_dir = .", APPID);
	
	UserData asr_data;

	ret = MSPLogin(NULL, NULL, login_config); //第一个参数为用户名，第二个参数为密码，传NULL即可，第三个参数是登录参数
	if (MSP_SUCCESS != ret)
	{
#if whether_print_log
		printf("[log]登录失败：%d\n", ret);
#endif
		result.whether_recognised = false;
		strcpy(result.fail_reason, "login error");
		goto exit;
	}
#if whether_print_log
		printf("[log]登录成功：%d\n", ret);
#endif
	memset(&asr_data, 0, sizeof(UserData));
#if whether_print_log
	printf("[log]构建离线识别语法网络...\n");
#endif
	ret = build_grammar(&asr_data); //第一次使用某语法进行识别，需要先构建语法网络，获取语法ID，之后使用此语法进行识别，无需再次构建
	if (MSP_SUCCESS !=ret)
	{
#if whether_print_log
		printf("[log]构建语法调用失败！\n");
#endif
		result.whether_recognised = false;
		strcpy(result.fail_reason, "build_grammer_error");
		goto exit;
	}
#if whether_print_log
		printf("[log]构建语法成功！\n");
#endif
	while (1 != asr_data.build_fini)
		usleep(300 * 1000);
	if (MSP_SUCCESS != asr_data.errcode)
		goto exit;
	ret = run_asr(&asr_data, path);
	if (MSP_SUCCESS != ret)
	{
#if whether_print_log
		printf("[log]离线语法识别出错: %d \n", ret);
#endif
		result.whether_recognised = false;
		strcpy(result.fail_reason, "bnf_detect_error");
		goto exit;
	}
#if whether_print_log
		printf("[log]离线语法识别成功: %d \n", ret);
#endif
	result.whether_recognised = true;
	strcpy(result.fail_reason, "　");
	if (strlen(whole_result) > 0)
	{
		result.whole_content = whole_result;
	}
	else
	{
		result.whole_content = " ";
	}
	goto exit;
exit:
	MSPLogout();
	return result;
}



