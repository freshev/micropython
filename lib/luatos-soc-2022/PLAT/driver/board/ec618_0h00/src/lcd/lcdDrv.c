#include "lcdDrv.h"
#include "slpman.h"

extern  void delay_us(uint32_t us);

void mDelay(uint32_t mDelay)
{
    delay_us(mDelay * 1000);
}

void lcdWriteData(uint8_t data)
{
    SPI_CS_LOW;
    LCD_DS_HIGH;
    SPI_SEND_DATA(data);
    //SPI_WAIT_TX_DONE;
    SPI_IS_BUSY;
    SPI_CS_HIGH;
}



void lcdWriteCmd(uint8_t cmd)
{
    SPI_CS_LOW;
    LCD_DS_LOW;
    SPI_SEND_DATA(cmd);
    SPI_WAIT_TX_DONE;
    SPI_CS_HIGH;
}

uint8_t lcdReadData()
{
    SPI_WAIT_TX_DONE;
    SPI_SEND_DATA(0xff); // Dummy data
    SPI_WAIT_TX_DONE;
    uint8_t data = SPI_READ_DATA;
    SPI_IS_BUSY;
    return data;
}

void lcdReadId()
{
#if (ST7789V2_ENABLE)
    uint8_t id1, id2, id3;

    LCD_RST_HIGH;
    mDelay(150); // Delay 200ms
    LCD_RST_LOW;
    mDelay(300); // Delay 400ms
    LCD_RST_HIGH;
    mDelay(400); // Delay 500ms

    lcdWriteData(0x04);
    (void)lcdReadData(); // Dummy
    id1 = lcdReadData();
    id2 = lcdReadData();
    id3 = lcdReadData();
    printf("LCD ID: %02x, %02x, %02x \r\n", id1, id2, id3);
#endif    
}

static void lcdGpioInit()
{
    PadConfig_t config;

    PAD_getDefaultConfig(&config);

    // Cs pin
    config.mux = SPI_CS_PAD_ALT_FUNC;
    PAD_setPinConfig(SPI_CS_PAD_ADDR, &config);

    GpioPinConfig_t gpioCfg;
    gpioCfg.pinDirection = GPIO_DIRECTION_OUTPUT;
    GPIO_pinConfig(SPI_CS_GPIO_INSTANCE, SPI_CS_GPIO_PIN, &gpioCfg);

    // Rst pin
    config.mux = LCD_RST_PAD_ALT_FUNC;
    PAD_setPinConfig(LCD_RST_PAD_ADDR, &config);

    gpioCfg.pinDirection = GPIO_DIRECTION_OUTPUT;
    //gpioCfg.misc.initOutput = 1;
    GPIO_pinConfig(LCD_RST_GPIO_INSTANCE, LCD_RST_GPIO_PIN, &gpioCfg);

    // Ds pin
    config.mux = LCD_DS_PAD_ALT_FUNC;
    PAD_setPinConfig(LCD_DS_PAD_ADDR, &config);

    gpioCfg.pinDirection = GPIO_DIRECTION_OUTPUT;
    GPIO_pinConfig(LCD_DS_GPIO_INSTANCE, LCD_DS_GPIO_PIN, &gpioCfg);

#if (ST7571_ENABLE)
    slpManAONIOVoltSet(IOVOLT_2_95V); // 54
    APmuWakeupPadSettings_t cfg;
    cfg.pullUpEn = 1;
    slpManSetWakeupPadCfg(WAKEUP_PAD_4, false, &cfg); //gpio21   50
	slpManAONIOPowerOn();//70
	

	// Lcd en pin
	config.mux = LCD_EN_PAD_ALT_FUNC;
    PAD_setPinConfig(LCD_EN_PAD_ADDR, &config);
    
	gpioCfg.pinDirection = GPIO_DIRECTION_OUTPUT;
	gpioCfg.misc.initOutput = 1;
    GPIO_pinConfig(LCD_EN_GPIO_INSTANCE, LCD_EN_GPIO_PIN, &gpioCfg);
#endif
}

static void lcdSpiInit()
{
    PadConfig_t config;

    PAD_getDefaultConfig(&config);

    config.mux = SPI_CLK_PAD_ALT_FUNC;
    PAD_setPinConfig(SPI_CLK_PAD_ADDR, &config);
    
    config.mux = SPI_MOSI_PAD_ALT_FUNC;
    PAD_setPinConfig(SPI_MOSI_PAD_ADDR, &config);
    
    config.mux = SPI_MISO_PAD_ALT_FUNC;
    PAD_setPinConfig(SPI_MISO_PAD_ADDR, &config);

    // Enable spi clock
    GPR_clockEnable(SPI_APB_CLOCK);
    GPR_clockEnable(SPI_FUNC_CLOCK);

    // Disable spi first
    SPI->CR1 = 0;

    // Pol = 0; PHA = 0; Data width = 8
    SPI->CR0 = 0x7;

    // lcd spi clock choose 26M by default to speed up the fps.
    CLOCK_clockEnable(CLK_HF51M); // open 51M
    CLOCK_setClockSrc(FCLK_SPI0, FCLK_SPI0_SEL_51M); // choose 51M
    SPI->CPSR = 2 & SPI_CPSR_CPSDVSR_Msk; // 2 division, to 26M
    SPI->CR0 = (SPI->CR0 & ~SPI_CR0_SCR_Msk) | 0;

    // Enable spi
    SPI->CR1 = SPI_CR1_SSE_Msk;
}

void lcdDispPic(uint8_t * pic)
{
#if (ST7789V2_ENABLE)
    displayPic_320x240(pic);
#elif  (ST7571_ENABLE)
	displayPic_60x80(pic);
#endif
}

void lcdClearScreen()
{
#if (ST7571_ENABLE)
	st7571CleanScreen();
#endif
}

void lcdInit(pTxCb txCb)
{
    lcdSpiInit();
    lcdGpioInit();
    
    lcdReadId();
#if (ST7789V2_ENABLE)
    st7789v2_init(txCb);
#endif

#if (ST7571_ENABLE)
	st7571_init();
	lcdClearScreen();
#endif    
}


