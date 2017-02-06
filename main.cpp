/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   main.cpp
 * Author: Mitch
 *
 * Created on 26 January 2017, 21:07
 */

#include <cstdlib>
#include "mcp3008Spi.h"
#include<stdio.h>
#include<bcm2835.h>

using namespace std;

#define MOTORFORWARDPIN RPI_V2_GPIO_P1_33
#define MOTORREVERSEPIN RPI_GPIO_P1_12
#define FWD_PWM_CHANNEL 0
#define REV_PWM_CHANNEL 1
#define RANGE 1024

#define A2DCHANNELFWD 0
#define A2DCHANNELREV 1

int FWD_val = -1;
int REV_val = -1;

mcp3008Spi a2d("/dev/spidev0.0", SPI_MODE_0, 1000000, 8);

static int measureSensor() // This measures both ADC channels and saves their values in FWD_val and REV_val
{
	unsigned char data[3];
	int a2dVal = 0;

	data[0] = 1; //  first byte transmitted -> start bit
	data[1] = 0b10000000 | (((A2DCHANNELFWD & 7) << 4)); // second byte transmitted -> (SGL/DIF = 1, D2=D1=D0=0)
	data[2] = 0; // third byte transmitted....don't care

	a2d.spiWriteRead(data, sizeof (data));

	a2dVal = 0;
	a2dVal = (data[1] << 8) & 0b1100000000; //merge data[1] & data[2] to get result
	a2dVal |= (data[2] & 0xff);
	FWD_val = a2dVal;
	
	
	data[0] = 1; //  first byte transmitted -> start bit
	data[1] = 0b10000000 | (((A2DCHANNELREV & 7) << 4)); // second byte transmitted -> (SGL/DIF = 1, D2=D1=D0=0)
	data[2] = 0; // third byte transmitted....don't care

	a2d.spiWriteRead(data, sizeof (data));

	a2dVal = 0;
	a2dVal = (data[1] << 8) & 0b1100000000; //merge data[1] & data[2] to get result
	a2dVal |= (data[2] & 0xff);
	REV_val = a2dVal;

	return 0;

}

static void motorForward()
{
	bcm2835_pwm_set_data(REV_PWM_CHANNEL, 0);
	while (FWD_val >= 30)
	{
		measureSensor();
		bcm2835_pwm_set_data(FWD_PWM_CHANNEL, FWD_val);
		cout << "Forward channel set to: " << FWD_val << endl;
		delay(100);
	}
	bcm2835_pwm_set_data(FWD_PWM_CHANNEL, 0);
}

static void motorReverse()
{
		bcm2835_pwm_set_data(FWD_PWM_CHANNEL, 0);
	while (REV_val >= 30)
	{
		measureSensor();
		bcm2835_pwm_set_data(REV_PWM_CHANNEL, REV_val);
		cout << "Reverse channel set to: " << REV_val << endl;
		delay(100);
	}
	bcm2835_pwm_set_data(REV_PWM_CHANNEL, 0);
}

int main()
{
	if (!bcm2835_init())
		return 1;
//--------------SETUP HERE----------------//
	bcm2835_gpio_fsel(MOTORFORWARDPIN, BCM2835_GPIO_FSEL_ALT0); //Make forward and reverse pins connected to respective PWM clocks.
	bcm2835_gpio_fsel(MOTORREVERSEPIN, BCM2835_GPIO_FSEL_ALT5);
	bcm2835_pwm_set_clock(BCM2835_PWM_CLOCK_DIVIDER_512); //Set clock divider.
	
	bcm2835_pwm_set_mode(FWD_PWM_CHANNEL, 1, 1); // Set mode and range for both forward and reverse channels
	bcm2835_pwm_set_range(FWD_PWM_CHANNEL, RANGE);
	
	bcm2835_pwm_set_mode(REV_PWM_CHANNEL, 1, 1);
	bcm2835_pwm_set_range(REV_PWM_CHANNEL, RANGE);
	
//---------------START OF MAIN LOOP---------//
	int i = 0;
	while (true)
	{
		measureSensor();
		//bcm2835_pwm_set_data(PWM_CHANNEL, a2dVal);
//		if (FWD_val >= 30)
//			motorForward();
//		else if (REV_val >= 30)
//			motorReverse();
		bcm2835_pwm_set_data(0, REV_val);
		bcm2835_pwm_set_data(1, FWD_val);
		if (i % 10000 == 0)
		{
			cout << "The sensor values are; Forward: " << FWD_val << " | Reverse: " << REV_val << endl;
			i = 0;
		}
		i++;
	}

	//bcm2835_pwm_set_data(PWM_CHANNEL, 0);
	bcm2835_close();

	return 0;
}

