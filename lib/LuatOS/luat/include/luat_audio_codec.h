
#ifndef __LUAT_AUDIO_CODEC_H__
#define __LUAT_AUDIO_CODEC_H__

typedef enum {
    LUAT_CODEC_SET_MUTE = 0,        //Mute setting
    LUAT_CODEC_GET_MUTE,            // Get the mute status
    LUAT_CODEC_SET_VOICE_VOL,       // volume setting
    LUAT_CODEC_GET_VOICE_VOL,       // Get the volume
    LUAT_CODEC_SET_MIC_VOL,         // mic volume setting
    LUAT_CODEC_GET_MIC_VOL,         // Get mic volume

    LUAT_CODEC_SET_FORMAT,          //codec data format setting
    LUAT_CODEC_SET_RATE,            //Sampling rate setting
    LUAT_CODEC_SET_BITS,            //Sampling bit setting
    LUAT_CODEC_SET_CHANNEL,         //Channel settings
    
    LUAT_CODEC_MODE_RESUME,
    LUAT_CODEC_MODE_STANDBY,
    LUAT_CODEC_MODE_PWRDOWN,
} luat_audio_codec_ctl_t;

typedef enum {
    LUAT_CODEC_MODE_SLAVE = 0,      //Default slave mode
    LUAT_CODEC_MODE_MASTER,

    LUAT_CODEC_MODE_ALL = 0,
    LUAT_CODEC_MODE_DAC,
    LUAT_CODEC_MODE_ADC,

    LUAT_CODEC_FORMAT_I2S = 0,
    LUAT_CODEC_FORMAT_LSB,
    LUAT_CODEC_FORMAT_MSB,
    LUAT_CODEC_FORMAT_PCMS,
    LUAT_CODEC_FORMAT_PCML,

}luat_audio_codec_ctl_param_t;

struct luat_audio_codec_opts;

typedef struct luat_audio_codec_conf {
    int i2c_id;                                                         // i2c id
    int i2s_id;                                                         // i2s id
    const struct luat_audio_codec_opts* codec_opts;                     // codec driver function
    uint8_t multimedia_id;                                              // multimedia id
} luat_audio_codec_conf_t;

typedef struct luat_audio_codec_opts{
    const char* name;
    int (*init)(luat_audio_codec_conf_t* conf,uint8_t mode);            //initialization
    int (*deinit)(luat_audio_codec_conf_t* conf);                       //Deinitialization
    int (*control)(luat_audio_codec_conf_t* conf,luat_audio_codec_ctl_t cmd,uint32_t data); //Control function
    int (*start)(luat_audio_codec_conf_t* conf);                        //stop
    int (*stop)(luat_audio_codec_conf_t* conf);                         //start
	uint8_t no_control;													//Cannot be adjusted, only switch
} luat_audio_codec_opts_t;

extern const luat_audio_codec_opts_t codec_opts_es8311;
extern const luat_audio_codec_opts_t codec_opts_tm8211;
extern const luat_audio_codec_opts_t codec_opts_common;
#endif
