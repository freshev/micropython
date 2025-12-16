

#include "cs_types.h"
#define OneWire_hwp_gpio 0x50107000

/**
 * @description: Set the single bus signal line to input pull-up
 * @param :pin{uint8}: pin to operate, optional ranges 0, 1, 2, 3, 7
 * @return: TRUE: Correct
 * FALSE: The pin is not in the allowable range*/
BOOL OneWire_IO_IN(uint8 pin)
{
    int *cfg = NULL;
    switch (pin)
    {
    case 0:
        cfg = 0x5010C098;
        break;
    case 1:
        cfg = 0x5010C09C;
        break;
    case 2:
        cfg = 0x5010C0A0;
        break;
    case 3:
        cfg = 0x5010C0A4;
        break;
    case 7:
        cfg = 0x5010C0B0;
        break;
    default:
        return FALSE;
    }
    *cfg = *cfg | 0x00010000;
    *cfg = *cfg | 0x00000200;

    int *hwp = NULL;
    hwp = OneWire_hwp_gpio + 4 * 7;
    *hwp = (1 << pin);
    hwp = OneWire_hwp_gpio + 4 * 15;
    *hwp = (1 << pin);
    hwp = OneWire_hwp_gpio + 4 * 17;
    *hwp = (1 << pin);
    hwp = OneWire_hwp_gpio + 4 * 19;
    *hwp = (1 << pin);

    hwp = OneWire_hwp_gpio + 4 * 2;
    *hwp = (1 << pin);
    return TRUE;
}

/**
 * @description: Set the single bus signal line to output, and the default output is high-quality flat
 * @param :pin{uint8}: pin to operate, optional ranges 0, 1, 2, 3, 7
 * @return: None*/
void OneWire_IO_OUT(uint8 pin)
{
    int *hwp = NULL;
    hwp = OneWire_hwp_gpio + 4 * 7;
    *hwp = (1 << pin);
    hwp = OneWire_hwp_gpio + 4 * 15;
    *hwp = (1 << pin);
    hwp = OneWire_hwp_gpio + 4 * 17;
    *hwp = (1 << pin);
    hwp = OneWire_hwp_gpio + 4 * 19;
    *hwp = (1 << pin);

    hwp = OneWire_hwp_gpio + 4 * 4;
    *hwp = (1 << pin);
    hwp = OneWire_hwp_gpio + 4 * 1;
    *hwp = (1 << pin);
}

/**
 * @description: Single bus output
 * @param : pin{uint8}: pin to operate, optional ranges 0, 1, 2, 3, 7
 * level{uint8}}: The level to be output. 0 or 1
 * @return: None*/
void OneWire_DQ_OUT(uint8 pin, uint8 level)
{
    if (level)
    {
        int *hwp = OneWire_hwp_gpio + 4 * 4;
        *hwp = (1 << pin);
    }
    else
    {
        int *hwp = OneWire_hwp_gpio + 4 * 5;
        *hwp = (1 << pin);
    }
}

/**
 * @description: Read the high and low level signal of a single bus
 * @param : pin{uint8}: pin to operate, optional ranges 0, 1, 2, 3, 7
 * @return: 0 or 1*/
bool OneWire_DQ_IN(uint8 pin)
{
    int *hwp = OneWire_hwp_gpio + 4 * 3;
    bool res = *hwp & (1 << pin);
    if (res)
        return 1;
    else
        return 0;
}

/**
 * @description: Single bus 1us delay function
 * @param : us{uint32}: Number of calls.
 * @return: None*/
#pragma GCC push_options
#pragma GCC optimize("O0")
void OneWire_Delay_1us(volatile uint32 us)
{
    if (us >= 0 && us < 5)
    {
        for (volatile int i = 15 * us; i > 0; i--)
        {
            ;
        }
    }
    else if (us >= 5 && us < 20)
    {
        for (volatile int i = 55 * us; i > 0; i--)
        {
            ;
        }
    }
    else if (us >= 20)
    {
        for (volatile int i = 62 * us; i > 0; i--)
        {
            ;
        }
    }
}
#pragma GCC push_options
