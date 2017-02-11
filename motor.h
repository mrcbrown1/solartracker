/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   motor.h
 * Author: Mitch
 *
 * Created on 11 February 2017, 00:59
 */

#ifndef MOTOR_H
#define MOTOR_H

class motor
{
public:
	void motorForwardSpeed(int speed);  // This function will turn the motor in the arbritrary forward direction
	void motorReverseSpeed(int speed); // This function will turn the motor in the arbritrary reverse direction	
	
	void disablePWMChannel(int PWMChannel); // This is to be used to disable a certain PWM channel
	void enablePWMChannel(int PWMChannel); // This is to enable a certain PWM channel
	
	int forwardPWM; // This holds the PWM channel that the forward pin is connected to
	int reversePWM; // This holds the PWM channel that the reverse pin is connected to

	// These are for setup
	motor(); // Default constructor, sets motor disable to true
	motor(int pwmChannelForward, int pwmChannelReverse); // Overload constructor, assigns correct values
	void setparams(int pwmChannelForward, int pwmChannelReverse); // For assigning parameters to motor after creating
private:
	bool disabled;  // True if motor should not run
	
};

#endif /* MOTOR_H */

