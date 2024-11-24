/**************************************************** ***************************
 * Multimedia device operation abstraction layer
 *************************************************** ****************************/
#ifndef __LUAT_MULTIMEDIA_H__
#define __LUAT_MULTIMEDIA_H__

#include "luat_base.h"
#include "luat_multimedia_codec.h"
#include "luat_audio.h"	//audio device
//reserved
// #include "luat_video.h" // Video device

//The following LUAT_MULTIMEDIA_AUDIO_BUS_XXX enumeration is only reserved, do not use it again, use LUAT_AUDIO_BUS_XXX
enum{
	LUAT_MULTIMEDIA_AUDIO_BUS_DAC=LUAT_AUDIO_BUS_DAC,
	LUAT_MULTIMEDIA_AUDIO_BUS_I2S=LUAT_AUDIO_BUS_I2S,
	LUAT_MULTIMEDIA_AUDIO_BUS_SOFT_DAC=LUAT_AUDIO_BUS_SOFT_DAC
};

// Multimedia related APIs are designed here

#endif
