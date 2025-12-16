#include "lcdDrv.h"

void st7789DispWindows();


#if USE_DMA_7789

int8_t st7789DmaTxCh; // dma tx channel
DmaDescriptor_t __ALIGNED(16) st7789DmaTxDesc[20];
bool isDMADone7789 = false;
typedef void (*st7789CbEvent_fn) (uint32_t event);     // st7789 callback event.
st7789CbEvent_fn st7789Cb;

static DmaTransferConfig_t lcdDmaTxCfg =
{
    NULL,
    (void *)&(SPI->DR),
    DMA_FLOW_CONTROL_TARGET,
    DMA_ADDRESS_INCREMENT_SOURCE,
    DMA_DATA_WIDTH_ONE_BYTE,
    DMA_BURST_8_BYTES, 
    0
};

static void st7789DmaEventCb(uint32_t event)
{
    switch(event)
    {
        case DMA_EVENT_END:
            if(st7789Cb)
            {
                st7789Cb(ST7789_EVENT_TRANSFER_COMPLETE);
            }
            
            break;
        case DMA_EVENT_ERROR:
        default:
            break;
    }
}

void st7789DmaInit()
{
    // Tx config
    DMA_init(DMA_INSTANCE_MP);
    st7789DmaTxCh = DMA_openChannel(DMA_INSTANCE_MP);

    DMA_setChannelRequestSource(DMA_INSTANCE_MP, st7789DmaTxCh, (DmaRequestSource_e)SPI_DMA_TX_REQID);
    DMA_rigisterChannelCallback(DMA_INSTANCE_MP, st7789DmaTxCh, st7789DmaEventCb);
}

#if 0
void lcdWriteSetup1(uint8_t * dataBuf, uint16_t dataCnt)
{
    // Configure tx DMA and start it
    lcdDmaTxCfg.sourceAddress = (void *)dataBuf;
    lcdDmaTxCfg.totalLength   = dataCnt; // every descriptor transfer this trunk of data
    DMA_buildDescriptorChain(st7789DmaTxDesc, &lcdDmaTxCfg, 20, true);

}
#endif
void lcdWriteSetup(uint8_t * dataBuf, uint32_t dataCnt)
{
    st7789DispWindows();

    // Configure tx DMA and start it
    lcdDmaTxCfg.sourceAddress = (void *)dataBuf;
    lcdDmaTxCfg.totalLength   = LCD_TRANSFER_SIZE_ONCE; // every descriptor transfer this trunk of data

    SPI_CS_LOW;
    LCD_DS_HIGH;
    DMA_buildDescriptorChain(st7789DmaTxDesc, &lcdDmaTxCfg, dataCnt/lcdDmaTxCfg.totalLength, true);//
    SPI_ENABLE_TX_DMA;
}

void lcdWriteCtrl(bool startOrStop)
{
    if (startOrStop)
    {
        DMA_loadChannelDescriptorAndRun(DMA_INSTANCE_MP, st7789DmaTxCh, st7789DmaTxDesc);
    }
    else
    {    
        extern void DMA_stopChannelNoWait(DmaInstance_e instance, uint32_t channel);
        DMA_stopChannelNoWait(DMA_INSTANCE_MP, st7789DmaTxCh);
        SPI_CS_HIGH;
    }
}


#endif




void lcdDispColor(uint16_t color)
{
#if USE_DMA_7789
    lcdWriteCtrl(true);
#else
    st7789DispWindows();
    int i, j;
    for (i = HEIGHT; i > 0; i--)
    {
        for (j = WIDTH; j > 0; j--)
        {
            lcdWriteData16(color);
        }
    }

#endif
}



void st7789DispWindows()
{
	lcdWriteCmd(0x2A);
	lcdWriteData(0x00);
	lcdWriteData(0x00);
	lcdWriteData(0x00);
	lcdWriteData(0xEF);

	lcdWriteCmd(0x2B);
	lcdWriteData(0x00);
	lcdWriteData(0x00);
	lcdWriteData(0x01);
	lcdWriteData(0x3f);
	lcdWriteCmd(0x2C);
}


void lcdWriteData16(uint16_t data)
{
    lcdWriteData(data >> 8);
    lcdWriteData(data);
}


void displayPic_320x240(uint8_t* pic)
{
#if USE_DMA_7789
    lcdWriteCtrl(true);
#else
    uint32_t i, j, k=0;

    st7789DispWindows();
    for (i = 0; i < 320; i++)
    {
        for (j = 0; j < 240; j++)
        {
            //lcdWriteData16(pic[j*2 + 320*i*2]);
            //lcdWriteData16(pic[j + WIDTH*i*2]);
            lcdWriteData(pic[k*2]);
            lcdWriteData(pic[k*2+1]);
            k++;
        }
    }
#endif
}


void st7789v2_init(pTxCb txCb)
{
    //--------------------------------ST7789V reset sequence------------------------------------//
    LCD_RST_HIGH;
    mDelay(50); //Delay 100ms
    LCD_RST_LOW;
    mDelay(150); //Delay 200ms
    LCD_RST_HIGH;
    mDelay(250); //Delay 500ms
    
    //-------------------------------Color Mode---------------------------------------------//
    lcdWriteCmd(0x11);
    mDelay (120); //Delay 120ms
    
    //--------------------------------Display Setting------------------------------------------//
    lcdWriteCmd(0x36);
    lcdWriteData(0x00);
    lcdWriteCmd(0x3a);
    lcdWriteData(0x05);
    
    //--------------------------------ST7789V Frame rate setting----------------------------------//
    lcdWriteCmd(0xb2);
    lcdWriteData(0x0c);
    lcdWriteData(0x0c);
    lcdWriteData(0x00);
    lcdWriteData(0x33);
    lcdWriteData(0x33);
    lcdWriteCmd(0xb7);
    lcdWriteData(0x35);
    
    //--------------------------------ST7789V Power setting--------------------------------------//
    lcdWriteCmd(0xbb);
    lcdWriteData(0x20);
    lcdWriteCmd(0xc0);
    lcdWriteData(0x2c);
    lcdWriteCmd(0xc2);
    lcdWriteData(0x01);
    lcdWriteCmd(0xc3);
    lcdWriteData(0x0b);
    lcdWriteCmd(0xc4);
    lcdWriteData(0x20);
    lcdWriteCmd(0xc6);
    lcdWriteData(0x0f);
    lcdWriteCmd(0xd0);
    lcdWriteData(0xa4);
    lcdWriteData(0xa1);
    
    //--------------------------------ST7789V gamma setting---------------------------------------//
    lcdWriteCmd(0xe0);
    lcdWriteData(0xd0);
    lcdWriteData(0x03);
    lcdWriteData(0x09);
    lcdWriteData(0x0e);
    lcdWriteData(0x11);
    lcdWriteData(0x3d);
    lcdWriteData(0x47);
    lcdWriteData(0x55);
    lcdWriteData(0x53);
    lcdWriteData(0x1a);
    lcdWriteData(0x16);
    lcdWriteData(0x14);
    lcdWriteData(0x1f);
    lcdWriteData(0x22);
    lcdWriteCmd(0xe1);
    lcdWriteData(0xd0);
    lcdWriteData(0x02);
    lcdWriteData(0x08);
    lcdWriteData(0x0d);
    lcdWriteData(0x12);
    lcdWriteData(0x2c);
    lcdWriteData(0x43);
    lcdWriteData(0x55);
    lcdWriteData(0x53);
    lcdWriteData(0x1e);
    lcdWriteData(0x1b);
    lcdWriteData(0x19);
    lcdWriteData(0x20);
    lcdWriteData(0x22);
    lcdWriteCmd(0x29);

#if USE_DMA_7789    
    st7789DmaInit(); 
    st7789Cb = txCb;
#endif
}


