#include <cstdlib>
#include "mcp3008Spi.h"
#include<stdio.h>
#include<bcm2835.h>
#include "mcp3008.h"
#include "motor.h"


using namespace std;

#define MOTORFORWARDPIN RPI_V2_GPIO_P1_33
#define MOTORREVERSEPIN RPI_GPIO_P1_12
#define FWD_PWM_CHANNEL 0
#define REV_PWM_CHANNEL 1
#define RANGE 1024


mcp3008Spi a2d("/dev/spidev0.0", SPI_MODE_0, 1000000, 8); // Making a global instance of the SPI interface to avoid creating/destroying too much

//static void motorForward()
//{
//	bcm2835_pwm_set_mode(REV_PWM_CHANNEL, 0, 0);
//	int i = 0;
//	while (FWD_val >= 30)
//	{
//		globalvars::measureSensor();
//		bcm2835_pwm_set_data(FWD_PWM_CHANNEL, FWD_val);
//		if (i % 15000 == 0) // Let's give diagnostics.
//		{
//			cout << "The motor is in forward motoring. Sensor value: " << FWD_val << endl;
//			i = 0;
//		}
//		i++;
//	}
//	bcm2835_pwm_set_data(FWD_PWM_CHANNEL, 0);
//	bcm2835_pwm_set_mode(REV_PWM_CHANNEL, 1, 1);
//}
//
//static void motorReverse()
//{
//	int i = 0;
//	bcm2835_pwm_set_mode(FWD_PWM_CHANNEL, 0, 0);
//	while (REV_val >= 30)
//	{
//		globalvars::measureSensor();
//		bcm2835_pwm_set_data(REV_PWM_CHANNEL, REV_val);
//		if (i % 15000 == 0) // Let's give diagnostics.
//		{
//			cout << "The motor is in reverse motoring. Sensor value: " << REV_val << endl;
//			i = 0;
//		}
//		i++;
//	}
//	bcm2835_pwm_set_data(REV_PWM_CHANNEL, 0);
//	bcm2835_pwm_set_mode(FWD_PWM_CHANNEL, 1, 1);
//}

int main()
{
	if (!bcm2835_init()) // Exit if BCM2835 isn't initialised
		return 1;

	//--------------SETUP HERE----------------//

	bcm2835_gpio_fsel(MOTORFORWARDPIN, BCM2835_GPIO_FSEL_ALT0); // Make forward and reverse pins connected to respective PWM clocks.
	bcm2835_gpio_fsel(MOTORREVERSEPIN, BCM2835_GPIO_FSEL_ALT5); // ALT5 is PWM channel 0, and ALT0 is PWM channel 1
	bcm2835_pwm_set_clock(BCM2835_PWM_CLOCK_DIVIDER_256); //Set clock divider : 1,2,4,8,16,32,64,128,256,512,1024,2048

	bcm2835_pwm_set_mode(FWD_PWM_CHANNEL, 1, 1); // Set mode and range for both forward and reverse channels
	bcm2835_pwm_set_range(FWD_PWM_CHANNEL, RANGE); // Format is (channel, mode, enabled) mode 1 = mark-space, 2 = balanced

	bcm2835_pwm_set_mode(REV_PWM_CHANNEL, 1, 1); // These are the same as above
	bcm2835_pwm_set_range(REV_PWM_CHANNEL, RANGE); // Channel is either 0 or 1, enabled is self explanatory

	mcp3008 myADC; // Create new instance of ADC object

	returnVars returnData; // returnVars is defined in mcp3008.h and is just the return data structure

	motor myMotor(0, 1);

	int fwsp, rvsp;

	//---------------START OF MAIN LOOP---------//

	int i = 0;

	while (true)
	{
		returnData = myADC.measureSensor(); // Grab the current ADC measurements
		fwsp = returnData.L0[0];
		rvsp = returnData.L0[1];

//		if (fwsp >= 30)
//		{
//			while (fwsp >= 30)
//			{
//				returnData = myADC.measureSensor();
//				fwsp = returnData.L0[0];
//				rvsp = returnData.L0[1];
//			}
//		}
//
//		elif(rvsp >= 30)
//		{
//
//		}
		if (i % 500 == 0) // Let's give diagnostics.
		{
			cout << string(100, '\n') << "The sensor values are; Forward: " << returnData.L0[0] << " | Reverse: " << returnData.L0[1] << endl;
			i = 0;
		}
		i++;

	}


	return 0;
}

