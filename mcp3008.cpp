/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   mcp3008.cpp
 * Author: Mitch
 * 
 * Created on 10 February 2017, 23:57
 */

#include "mcp3008.h"
#include "mcp3008Spi.h"

extern mcp3008Spi a2d;

returnVars mcp3008::measureSensor()
{
	returnVars data;
	for (int i = 0; i <= 7; i++)
	{
		data.L0[i] = measureInput(i);
//		std::cout << "Testing ADC channel: " << i << " | Value returned is: " << data.L0[i] << std::endl; // This is for debugging only
	}
	return data;
}

int mcp3008::measureInput(int channel)// This measures both ADC channels and saves their values in FWD_val and REV_val
{
	unsigned char data[3];
	int a2dVal = 0;

	data[0] = 1; //  first byte transmitted -> start bit
	data[1] = 0b10000000 | (((channel & 7) << 4)); // second byte transmitted -> (SGL/DIF = 1, D2=D1=D0=0)
	data[2] = 0; // third byte transmitted....don't care

	a2d.spiWriteRead(data, sizeof (data));

	a2dVal = 0;
	a2dVal = (data[1] << 8) & 0b1100000000; //merge data[1] & data[2] to get result
	a2dVal |= (data[2] & 0xff);

	return a2dVal;

}
