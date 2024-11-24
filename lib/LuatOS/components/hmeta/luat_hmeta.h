#ifndef LUAT_HMETA_H
#define LUAT_HMETA_H


// Get the device type of the Modules. The original requirement is to distinguish between Air780E and Air600E
int luat_hmeta_model_name(char* buff);
// Get the hardware version number, such as A11, A14
int luat_hmeta_hwversion(char* buff);


// -------------------------------------------------------
// GPIO metadata
// -------------------------------------------------------
typedef struct luat_hmeta_gpio_pin
{
    uint16_t id;            // GPIO number
    // const char* name; // pin name
    uint8_t pull_up : 1;    //Support pull-up
    uint8_t pull_down: 1;   //Support drop-down
    uint8_t pull_open: 1;   //Support open drain
    uint8_t irq_rasing:1;   //Support rising edge interrupt
    uint8_t irq_falling:1;  //Support falling edge interrupt
    uint8_t irq_both   :1;  //Support bidirectional triggering
    uint8_t irq_high   :1;  //Support high level trigger
    uint8_t irq_low   :1;  //Support low level trigger
    uint8_t volsel;         // voltage range
    const char* commet;     //Remarks information
}luat_hmeta_gpio_pin_t;

typedef struct luat_hmeta_gpio
{
    size_t count;      //How many pins are there in total?
    const char* comment;   // General remarks
    const luat_hmeta_gpio_pin_t pins;
}luat_hmeta_gpio_t;

const luat_hmeta_gpio_t* luat_hmeta_gpio_get(void);

// -------------------------------------------------------
// UART metadata
// -------------------------------------------------------
typedef struct luat_hmeta_uart_port
{
    uint16_t id;            // UART number
    uint16_t pins[2];       //Corresponding GPIO, if any
    uint16_t baudrates[16]; //Available baud rate, 0 will be automatically skipped
    const char* commet;     //Remarks information
}luat_hmeta_uart_port_t;

typedef struct luat_hmeta_uart
{
    size_t count;      //How many pins are there in total?
    const char* comment;   // General remarks
    const luat_hmeta_uart_port_t ports;
}luat_hmeta_uart_t;

const luat_hmeta_uart_t* luat_hmeta_uart_get(void);

// TODO i2c/spi/pwm/adc metadata

#endif
