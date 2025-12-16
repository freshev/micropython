/**
 * @file audio_ll_drv.h
 * @brief The middle layer where audio data is connected to the underlying hardware driver is not optimal in order to be compatible with various situations. Since the hardware driver code is open source, users can not use it and connect the driver code themselves.
 * @version 0.1
 * @date 2022-10-23
 *
 * @copyright
 **/
#ifndef __AUDIO_LL_DRV_H__
#define __AUDIO_LL_DRV_H__

typedef enum
{
    AUSTREAM_FORMAT_UNKNOWN, ///< placeholder for unknown format
    AUSTREAM_FORMAT_PCM,     ///< raw PCM data
    AUSTREAM_FORMAT_WAVPCM,  ///< WAV, PCM inside
    AUSTREAM_FORMAT_MP3,     ///< MP3
    AUSTREAM_FORMAT_AMRNB,   ///< AMR-NB
    AUSTREAM_FORMAT_AMRWB,   ///< AMR_WB
    AUSTREAM_FORMAT_SBC,     ///< bt SBC
} auStreamFormat_t;

typedef enum
{
    AUSTREAM_BUS_DAC,
	AUSTREAM_BUS_I2S,
	AUSTREAM_BUS_SOFT_DAC,	//PWM analog DAC
} auStreamBusType_t;

typedef struct
{
	CBFuncEx_t CB;	//pData is its own Audio_StreamStruct pointer
	CBFuncEx_t Decoder;
	CBFuncEx_t Encoder;
	void *pParam;
	void *fd;
	void *CoderParam;
	void *UserParam;
	Buffer_Struct FileDataBuffer;
	Buffer_Struct AudioDataBuffer;
	Buffer_Struct RomDataBuffer;
	llist_head DataHead;
	uint32_t SampleRate;
	uint32_t waitRequire;
	uint8_t BitDepth;
	uint8_t ChannelCount;	//Channel, currently only 1 or 2
	auStreamFormat_t Format;
	auStreamBusType_t BusType;	//Audio bus type, DAC, IIS, etc.
	uint8_t BusID;		//Audio bus ID
	uint8_t IsDataSigned;	//Whether the data is signed
	uint8_t IsHardwareRun;
	uint8_t IsPause;
	uint8_t IsStop;
	uint8_t IsPlaying;
	uint8_t IsFileNotEnd;
	uint8_t DecodeStep;
}Audio_StreamStruct;

/**
 * @brief audio driver initialization
 **/
void Audio_GlobalInit(void);
/**
 * @brief The I2S bus mode configuration related to the external codec based on the I2S bus does not handle the I2C of the codec
 *
 * @param bus_id I2S bus serial number, 0 or 1
 * @param mode I2S mode, I2S_MODE_I2S, I2S_MODE_MSB, I2S_MODE_LSB, generally common MODE_I2S and MODE_MSB
 * @param frame_size I2S frame mode, I2S_FRAME_SIZE_16_16, I2S_FRAME_SIZE_16_32, I2S_FRAME_SIZE_24_32, I2S_FRAME_SIZE_32_32, currently only used
 *I2S_FRAME_SIZE_16_16*/
void Audio_CodecI2SInit(uint8_t bus_id, uint8_t mode,  uint8_t frame_size);

/**
 * @brief Start playing the original audio stream
 *
 * @param pStream original audio stream data structure, the bottom layer does not save this structure and needs to be saved by the user
 * @return =0 success < 0 failure error code*/
int32_t Audio_StartRaw(Audio_StreamStruct *pStream);
/**
 * @brief writes original audio data to the bottom layer
 *
 * @param pStream original audio stream data structure
 * @param pByteData original audio data
 * @param ByteLen Original audio data byte length
 * @param AddHead Whether to insert to the beginning of the playback sequence, 1 is inserted at the beginning, 0 is inserted at the end. Generally used to insert blank segments when starting/resuming playback to eliminate plosive sounds. Do not insert normal data at the beginning.
 * @return int32_t =0 success < 0 failure error code*/
int32_t Audio_WriteRaw(Audio_StreamStruct *pStream, uint8_t *pByteData, uint32_t ByteLen, uint8_t AddHead);
/**
 * @brief Terminate audio data playback
 *
 * @param pStream original audio stream data structure*/
void Audio_Stop(Audio_StreamStruct *pStream);
/**
 * @brief Clear the data after playing, which needs to be done in the task
 *
 * @param pStream original audio stream data structure*/
void Audio_DeleteOldData(Audio_StreamStruct *pStream);
/**
 * @brief pauses the playback of audio data, just sets the stop flag, and the data already in the driver will still be played
 *
 * @param pStream original audio stream data structure*/
void Audio_Pause(Audio_StreamStruct *pStream);
/**
 * @brief Resume playing audio data
 *
 * @param pStream original audio stream data structure*/
void Audio_Resume(Audio_StreamStruct *pStream);

#endif
