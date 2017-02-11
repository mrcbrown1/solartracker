/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   motor.cpp
 * Author: Mitch
 * 
 * Created on 11 February 2017, 00:59
 */

#include "motor.h"


void motor::motorForwardSpeed(int speed)
{
	if(!disabled)
	{
		
	}
}

void motor::motorReverseSpeed(int speed)
{
	if(!disabled)
	{
		
	}
}

void motor::disablePWMChannel(int PWMChannel)
{
	if(!disabled)
	{
		bcm2835_pwm_set_mode(PWMChannel, 1, 0);
	}
	
}

void motor::enablePWMChannel(int PWMChannel)
{
	if(!disabled)
	{
		bcm2835_pwm_set_mode(PWMChannel, 1, 1);
	}
}

motor::motor()
{
	disabled = true;
}

motor::motor(int pwmChannelForward, int pwmChannelReverse)
{
	disabled = false;
	forwardPWM = pwmChannelForward;
	reversePWM = pwmChannelReverse;
}

void motor::setparams(int pwmChannelForward, int pwmChannelReverse)
{
	disabled = false;
	forwardPWM = pwmChannelForward;
	reversePWM = pwmChannelReverse;
}