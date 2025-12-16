/***************
	demo_hello
****************/
#include <string.h>
#include "iot_debug.h"
#include "iot_os.h"

#include "am_openat_drv.h"
#include "iot_audio.h"

#include "iot_vat.h"

char audiobuf[1024 * 300] = {0};
uint32 recvbufsize = 0;

void audio_rec_handle(E_AMOPENAT_RECORD_ERROR result)
{
    iot_debug_print("[poc-audio] audio_rec_handle result:%d", result);
}

void audio_streamplay_handle(E_AMOPENAT_PLAY_ERROR result)
{
    iot_debug_print("[poc-audio] audio_streamplay_handle result:%d", result);
}

void audio_stream_record_cb(int ret, char *data, int len)
{
    //iot_debug_print("[poc-audio] audio_stream_record_cb ret:%d len:%d", ret, len);
    memcpy(audiobuf + recvbufsize, data, len); //Á÷Êý¾Ý×ª´æ
    recvbufsize += len;
}

void poc_audio_stream_record()
{
    recvbufsize = 0;
    iot_debug_print("[poc-audio] poc_audio_stream_record start ");
    E_AMOPENAT_RECORD_PARAM param;
    param.record_mode = OPENAT_RECORD_STREAM; // ê¹óÃe ÷ vê¼òÔ ½½½
    param.quality = OPENAT_RECORD_QUALITY_MEDIUM;
    param.type = OPENAT_RECORD_TYPE_POC;  // ö »óðñ¡ôñPocààðn²å¾ßóðïûôë¹¦äü
    param.format = OPENAT_AUD_FORMAT_PCM; // poc û Ã Ã Ã Ã ä Ã Ã Ã Ã Ã Ã Ã Ã »Ã Ã Ã ä Ã Ã Ã» Ã Ã Ã Ã Ã »Ã Ã Ã »p Ã Ã Ã »½ Ã Ã Ã Ã »½ ä Ã Ã Ã »½ ä
    param.time_sec = 0;
    param.stream_record_cb = audio_stream_record_cb; //Á÷Êý¾Ý»Øµ÷º¯Êý
    iot_audio_rec_start(&param, audio_rec_handle);
}

void poc_audio_test(PVOID pParameter)
{
    iot_debug_set_fault_mode(OPENAT_FAULT_HANG);

    //´ò¿ªµ÷ÊÔÐÅÏ¢£¬Ä¬ÈÏ¹Ø±Õ
    iot_vat_send_cmd((UINT8 *)"AT^TRACECTRL=0,1,1\r\n", sizeof("AT^TRACECTRL=0,1,1\r\n"));

    //GPPO15É115É1 »²ß²²²²²²²»
    T_AMOPENAT_GPIO_CFG output_cfg = {0};
    output_cfg.mode = OPENAT_GPIO_OUTPUT; // Åäöãêä³ö
    output_cfg.param.defaultState = 1;    // Ä¬ÈÏ¸ßµçÆ½
    // gpio0³õÊ¼»¯
    iot_gpio_open(15, &output_cfg);
    iot_audio_set_speaker_vol(90);                       // As well as it
    iot_audio_set_channel(OPENAT_AUDIOHAL_ITF_RECEIVER); // éè² ·é à
    HANDLE g_demo_timer1 = iot_os_create_timer((PTIMER_EXPFUNC)poc_audio_stream_record, NULL);
    // ½ "gpio0éöãîªßµçæ½
    while (1)
    {
        iot_gpio_set(15, 0); // â¼òôê ± ¹ ± õ shade · to £
        // 1¡¢1sºóÆô¶¯Á÷Â¼Òô
        iot_os_start_timer(g_demo_timer1, 1000);
        iot_os_sleep(5 * 1000); // 5Shoó¹ø ± Õ Õ¼ôô
        //2¡¢¹Ø±ÕÂ¼Òô
        iot_audio_rec_stop();
        //3¡¢Á÷²¥·ÅÂ¼Òô
        iot_debug_print("[poc-audio] recvbufsize: %d", recvbufsize);
        iot_gpio_set(15, 1); // You're Turn¸ã¸¸¸ çæ½a's'o¿â yet tooÃna
        int plen = 0;
        while (plen <= recvbufsize)
        {
            //ÏûÔë×¨ÓÃ£¬Á÷²¥·Å½Ó¿Ú£¬PCM¸ñÊ½Ò»´Î×î¶à²¥·Å4096×Ö½Ú
            int len = iot_audio_streamplayV2(OPENAT_AUD_PLAY_TYPE_POC, OPENAT_AUD_FORMAT_PCM, audio_streamplay_handle, audiobuf + plen, recvbufsize / 15);
            plen += len;
            iot_debug_print("[poc-audio] iot_audio_streamplay len: %d plen:%d", len, plen);
            iot_os_sleep(280); // × Ó¼ºº —øöæê ± ¼ £ ¬e ÷ ² ¥ · å½ó ° × × èèû½ó £ £ £
        }
        iot_debug_print("[poc-audio] iot_audio_streamplay over");
        // 4¡ ¢ In £ Ö¹² ¥ · å
        iot_audio_stop_music();
    }
    return 0;
}


int appimg_enter(void *param)
{
    iot_debug_print("[hello]appimg_enter");
    // ¹
    iot_debug_set_fault_mode(OPENAT_FAULT_HANG);
    iot_os_sleep(100);
    // odleypad, μúmèwork £ уоæâÂеtAtA ¿А е½ü Â © е½
    // POCÏÎÄ¿Ã »ÒÝ³ÖBOOT ° ´¼Ü £ ¬ ± ØÐËµ ÷ óÃÒ» ´ÎÉÈÖKPLAD½ØÈËÏÂÔØÄ £ Ê½ £ ¬ · ñÔÒ ± Ä × © ºóÖ »ÄÜ²Ð» úeË¡ £
    iot_vat_send_cmd("AT*DOWNLOAD=2,3,2\r\n", sizeof("AT*DOWNLOAD=2,3,2\r\n"));
    iot_os_sleep(100);
    //´ò¿ªµ÷ÊÔÐÅÏ¢£¬Ä¬ÈÏ¹Ø±Õ
    iot_vat_send_cmd("AT^TRACECTRL=0,1,2\r\n", sizeof("AT^TRACECTRL=0,1,2\r\n"));
    iot_os_sleep(100);

    iot_os_create_task(poc_audio_test, NULL, 1024, 1, OPENAT_OS_CREATE_DEFAULT, "poc_audio_test");
    return 0;
}

void appimg_exit(void)
{
    iot_debug_print("[poc-audio] appimg_exit");
}
