#ifndef LUAT_I2S_H
#define LUAT_I2S_H 

enum {
    LUAT_I2S_MODE_MASTER = 0,   // Host mode
    LUAT_I2S_MODE_SLAVE,        // slave mode

    LUAT_I2S_MODE_I2S = 0,      // I2S standard
    LUAT_I2S_MODE_LSB,          // LSB standard
    LUAT_I2S_MODE_MSB,          // MSB standard
    LUAT_I2S_MODE_PCMS,         // PCM short frame standard
    LUAT_I2S_MODE_PCML,         // PCM long frame standard

    LUAT_I2S_CHANNEL_LEFT = 0,  // left channel
    LUAT_I2S_CHANNEL_RIGHT,     //right channel
    LUAT_I2S_CHANNEL_STEREO,    // stereo

    LUAT_I2S_BITS_16 = 16,      // 16-bit data
    LUAT_I2S_BITS_24 = 24,      // 24-bit data
    LUAT_I2S_BITS_32 = 32,      // 32-bit data

    LUAT_I2S_HZ_8k  = 8000,     // i2s 8kHz sampling rate
    LUAT_I2S_HZ_11k = 11000,    // i2s 11kHz sampling rate
    LUAT_I2S_HZ_16k = 16000,    // i2s 16kHz sampling rate
    LUAT_I2S_HZ_22k = 22050,    //i2s 22.05kHz sampling rate
    LUAT_I2S_HZ_32k = 32000,    // i2s 32kHz sampling rate
    LUAT_I2S_HZ_44k = 44100,    //i2s 44.1kHz sampling rate
    LUAT_I2S_HZ_48k = 48000,    // i2s 48kHz sampling rate
    LUAT_I2S_HZ_96k = 96000,    // i2s 96kHz sampling rate

    LUAT_I2S_STATE_STOP = 0,    // i2s stop state
    LUAT_I2S_STATE_RUNING,      // i2s transmission status
};

typedef enum {
    LUAT_I2S_EVENT_TX_DONE,
    LUAT_I2S_EVENT_TX_ERR,
    LUAT_I2S_EVENT_RX_DONE,
    LUAT_I2S_EVENT_RX_ERR,
    LUAT_I2S_EVENT_TRANSFER_DONE,
    LUAT_I2S_EVENT_TRANSFER_ERR,
} luat_i2s_event_t;

typedef struct luat_i2s_conf{
    uint8_t id;                                             // i2s id
    uint8_t mode;                                           // i2s mode
    uint8_t standard;                                       //i2s data standard
    uint8_t channel_format;                                 //i2s channel format
    uint8_t data_bits;                                      //i2s valid data digits
    uint8_t channel_bits;                                   //i2s channel data bits
    volatile uint8_t state;                                 // i2s status
    uint8_t is_full_duplex;		                            // Whether full duplex
    uint32_t sample_rate;                                   // i2s sampling rate
    uint32_t cb_rx_len;                                     //Receive trigger callback data length
    int (*luat_i2s_event_callback)(uint8_t id ,luat_i2s_event_t event, uint8_t *rx_data, uint32_t rx_len, void *param); // i2s callback function
    void *userdata;                                         //User data
}luat_i2s_conf_t;

//Configuration
int luat_i2s_setup(const luat_i2s_conf_t *conf);                  //Initialize i2s
int luat_i2s_modify(uint8_t id,uint8_t channel_format,uint8_t data_bits,uint32_t sample_rate);      // Modify i2s configuration (no initialization operation will be performed, configuration will be modified dynamically)
//Transmission (asynchronous interface)
int luat_i2s_send(uint8_t id, uint8_t* buff, size_t len);                                   // i2s sends data
int luat_i2s_recv(uint8_t id, uint8_t* buff, size_t len);                                   // i2s receives data
int luat_i2s_transfer(uint8_t id, uint8_t* txbuff, size_t len);                             //i2s transmit data (full duplex)
int luat_i2s_transfer_loop(uint8_t id, uint8_t* buff, uint32_t one_truck_byte_len, uint32_t total_trunk_cnt, uint8_t need_callback);   // i2s cyclically transmits data (full duplex)
// control
int luat_i2s_pause(uint8_t id);                 // i2s transfer paused
int luat_i2s_resume(uint8_t id);                // i2s transmission recovery
int luat_i2s_close(uint8_t id);                 // i2s close

// Get configuration
luat_i2s_conf_t *luat_i2s_get_config(uint8_t id);

int luat_i2s_txbuff_info(uint8_t id, size_t *buffsize, size_t* remain);
int luat_i2s_rxbuff_info(uint8_t id, size_t *buffsize, size_t* remain);

int luat_i2s_set_user_data(uint8_t id, void *user_data);
#endif
