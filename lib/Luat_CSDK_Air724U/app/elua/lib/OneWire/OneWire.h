#ifndef __ONEWIRE_H
#define __ONEWIRE_H


#include "am_openat.h"

/**
 * @description: Output debug information*/
#define OneWire_printf OPENAT_print

/**
 * @description: Set the single bus signal line as input. Pull-up by default
 * @param :pin{uint8}: pin to operate, optional ranges 0, 1, 2, 3, 7
 * @return: TRUE: Correct
 * FALSE: The pin is not in the allowable range*/
BOOL OneWire_IO_IN(uint8 pin);

/**
 * @description: Set the single bus signal line to output. Default output high-quality flat
 * @param :pin{uint8}: pin to operate, optional ranges 0, 1, 2, 3, 7
 * @return: None*/
void OneWire_IO_OUT(uint8 pin);

/**
 * @description: Single bus output high and low level
 * @param : pin{uint8}: pin to operate, optional ranges 0, 1, 2, 3, 7
 * level{uint8}}: The level to be output. 0 or 1
 * @return: None*/
void OneWire_DQ_OUT(uint8 pin, uint8 level);

/**
 * @description: Read the high and low level signal of a single bus
 * @param : pin{uint8}: pin to operate, optional ranges 0, 1, 2, 3, 7
 * @return: 0 or 1*/
bool OneWire_DQ_IN(uint8 pin);

/**
 * @description: Single bus 1us delay function
 * @param : us{uint32}: Number of calls.
 * @return: None*/
void OneWire_Delay_1us(volatile uint32 us);
#endif
