
#include "ds18b20.h"
#include "stdio.h"
#include "stdlib.h"
static uint8 ds18b20_data = -1;

static void DS18B20_Rst(void)
{
    OneWire_IO_OUT(ds18b20_data);
    OneWire_DQ_OUT(ds18b20_data, 0);
    OneWire_Delay_1us(600);
    OneWire_DQ_OUT(ds18b20_data, 1);
    OneWire_Delay_1us(10);
}

static uint8 DS18B20_Check(void)
{
    uint8 retry = 0;
    OneWire_IO_IN(ds18b20_data);
    OneWire_Delay_1us(10);
    while (OneWire_DQ_IN(ds18b20_data) && retry < 12)
    {
        retry++;
        OneWire_Delay_1us(20);
    }
    if (retry >= 12)
    {
        OneWire_printf("[ds18b20]DS18B20_Check: retry1 >= 12!");
        return 3;
    }

    retry = 0;
    while (!OneWire_DQ_IN(ds18b20_data) && retry < 12)
    {
        retry++;
        OneWire_Delay_1us(20);
    }
    if (retry >= 12)
    {
        OneWire_printf("[ds18b20]DS18B20_Check: retry2 >= 12!");
        return 3;
    }
    return 0;
}

static uint8 DS18B20_Read_Bit(void)
{
    uint8 data = 0;
    OneWire_IO_OUT(ds18b20_data);
    OneWire_DQ_OUT(ds18b20_data, 0);
    OneWire_Delay_1us(5);

    OneWire_IO_IN(ds18b20_data);
    OneWire_Delay_1us(10);
    if (OneWire_DQ_IN(ds18b20_data))
        data = 1;
    else
        data = 0;
    OneWire_Delay_1us(60);
    return data;
}

static uint8 DS18B20_Read_Byte(void)
{
    uint8 i, j, dat;
    dat = 0;
    for (i = 1; i <= 8; i++)
    {
        j = DS18B20_Read_Bit();
        dat = (j << 7) | (dat >> 1);
    }
    return dat;
}

static void DS18B20_Write_Byte(uint8 dat)
{
    uint8 j;
    uint8 testb;
    OneWire_IO_OUT(ds18b20_data);
    for (j = 1; j <= 8; j++)
    {
        testb = dat & 0x01;
        dat = dat >> 1;
        if (testb)
        {
            OneWire_DQ_OUT(ds18b20_data, 0);
            OneWire_Delay_1us(5);
            OneWire_DQ_OUT(ds18b20_data, 1);
            OneWire_Delay_1us(60);
        }
        else
        {
            OneWire_DQ_OUT(ds18b20_data, 0);
            OneWire_Delay_1us(60);
            OneWire_DQ_OUT(ds18b20_data, 1);
            OneWire_Delay_1us(5);
        }
    }
}

static uint8 DS18B20_Start(void)
{
    DS18B20_Rst();
    uint8 var = DS18B20_Check();
    if (var != 0)
    {
        return var;
    }
    DS18B20_Write_Byte(0xcc);
    DS18B20_Write_Byte(0x44);
    return 0;
}

static uint8 DS18B20_Colle(uint8 *LSB, uint8 *MSB)
{
    uint8 var = DS18B20_Start();
    if (var != 0)
    {
        return var;
    }
    OneWire_Delay_1us(40);
    DS18B20_Rst();
    var = DS18B20_Check();
    if (var != 0)
    {
        return var;
    }
    DS18B20_Write_Byte(0xcc);
    DS18B20_Write_Byte(0xbe);
    *LSB = DS18B20_Read_Byte();
    *MSB = DS18B20_Read_Byte();
    return 0;
}

/**
 * @dwriting: ugds18b20µµµ´â ¼¾¾¾µ²0.0625¹´´«´´óR³
 * @param : pin{unt8}:O´µµý´´µug¶ç0§0±±±±¢7
 * TempNum{int *: ISâ uggâ₳
 * @return 0:Õ³£”.
 * 2:´ÀµµµnÔÔÔnnnnnS§
 * 3:â€™s²µ²µµµ´s*/
uint8 DS18B20_GetTemp_Num(uint8 pin, int *TempNum)
{
    HANDLE crihand=NULL;
    uint8 LSB = 0, MSB = 0;
    uint8 var = 0;
    if (pin == 0 || pin == 1 || pin == 2 || pin == 3 || pin == 7)
    {
        ds18b20_data = pin;
        goto start;
    }
    return 2;

start:
    crihand = OPENAT_enter_critical_section();
    var = DS18B20_Colle(&LSB, &MSB);
    OPENAT_exit_critical_section(crihand);
    if (var != 0)
    {
        return var;
    }
    //OneWire_printf("[ds18b20]DS18B20_Get_Temp: LSB:%d", LSB);
    //OneWire_printf("[ds18b20]DS18B20_Get_Temp: MSB:%d", MSB);
    uint8 flag = 0;
    if (MSB > 7)
    {
        flag = 1;
    }

    int data = 0;
    data = MSB;
    data <<= 8;
    data += LSB;
    int temp = 0;
    if (flag == 1)
    {
        temp = (((~data) & (0x07FF)) * 625);
    }
    else
    {
        temp = ((data & (0x07FF)) * 625);
    }

    if (flag)
        *TempNum = 0 - temp;
    else
        *TempNum = temp;

    ds18b20_data = -1;
    return 0;
}

/*you
 I
 I
 I
 *@re
 * 1:TSTS
 *2:2µµ »
 * 3: ¼ón¬¬¬¬18b20*/
uint8 DS18B20_GetTemp_String(uint8 pin, char *TempStr)
{
    if (TempStr == NULL)
    {
        return 1;
    }

    int TempNum = 0;
    uint8 var = DS18B20_GetTemp_Num(pin, &TempNum);
    if (var != 0)
    {
        return var;
    }
    int data1 = TempNum / 10000;
    int data2 = TempNum % 10000;
    char str1[10] = {0};
    itoa(data1, str1, 10);
    char str2[10] = {0};
    itoa(data2, str2, 10);
    sprintf(TempStr, "%s.%s C", str1, str2);
    return 0;
}
