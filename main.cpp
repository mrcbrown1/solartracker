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

using namespace std;
#include<stdio.h>
#include<bcm2835.h>
#define PIN RPI_GPIO_P1_12
#define PWM_CHANNEL 0
#define RANGE 1024
/*
 * 
 */
int main(int argc, char** argv)
{
	if (!bcm2835_init())
		return 1;
	bcm2835_gpio_fsel(PIN, BCM2835_GPIO_FSEL_ALT5);
	
	bcm2835_pwm_set_clock(BCM2835_PWM_CLOCK_DIVIDER_16);
	bcm2835_pwm_set_mode(PWM_CHANNEL, 1, 1);
	bcm2835_pwm_set_range(PWM_CHANNEL, RANGE);
	
	int direction = 1;
	int data = 1;
	for (int i = 0; i<10;)
	{
		if (data == 1){
			direction = 1;
			i++;
		}
		else if (data == RANGE-1){
			direction = -1;
//			i++;
		}			
		data += direction;
		bcm2835_pwm_set_data(PWM_CHANNEL, data);
		bcm2835_delay(1);
	}
	bcm2835_pwm_set_data(PWM_CHANNEL, 0);
	bcm2835_close();
	return 0;
}