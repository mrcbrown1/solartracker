#include <cstdlib>
#include "mcp3008Spi.h"
#include <stdio.h>
#include <bcm2835.h>
#include "mcp3008.h"
#include <ios>
#include <fstream>
#include <time.h>
#include <sstream>
#include <string>



using namespace std;

#define MOTORFORWARDPIN RPI_V2_GPIO_P1_33
#define MOTORREVERSEPIN RPI_GPIO_P1_12
#define FWD_PWM_CHANNEL 1
#define REV_PWM_CHANNEL 0
#define RANGE 1024

int fwsp, rvsp;
time_t now = time(0);
int day, month, year;
const int tstart = time(0);
int tlast = tstart;
mcp3008 myADC; // Create new instance of ADC object

returnVars returnData; // returnVars is defined in mcp3008.h and is just the return data structure

mcp3008Spi a2d("/dev/spidev0.0", SPI_MODE_0, 1000000, 8); // Making a global instance of the SPI interface to avoid creating/destroying too much

static void logData(int voltage, float current)
{
	now = time(0); //Get the current time
	tm *ltm = localtime(&now); // Convert it into tm struct for easy access
	
	day = ltm->tm_mday; // Extract the day, month and year
	month = ltm->tm_mon;
	year = 1900 + ltm->tm_year;
	
	string varname = to_string(day) + "-" + to_string(month) + "-" + to_string(year); // Make a unique filename for the log
	string path="/root/solardata/data-" + varname + ".csv";
    ofstream file(path, std::ios::app); //open in constructor, make sure we append data
    file << now-tstart << "," << voltage << "," << current << endl; // Echo in the correct data, return to main loop, destructor auto called
}

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
	bcm2835_pwm_set_data(FWD_PWM_CHANNEL, 0); // Reset PWM channel to zero incase there was leftover data

	bcm2835_pwm_set_mode(REV_PWM_CHANNEL, 1, 1); // These are the same as above
	bcm2835_pwm_set_range(REV_PWM_CHANNEL, RANGE); // Channel is either 0 or 1, enabled is self explanatory
	bcm2835_pwm_set_data(REV_PWM_CHANNEL, 0); // Reset PWM channel to zero incase there was leftover data


	//---------------START OF MAIN LOOP---------//

	int i = 0; // int to act as counter for debugging
	int v_meas = 0; // int to hold the measured voltage from the ADC 0-1024
	float v_pred = 0; // float to hold decimal approximation of voltage
	
	
	int i_meas = 0; // int to hold the measured current from the ADC 0-1024
	float i_pred = 0; // float to hold decimal approximation of the current
	
		while (true)
		{
			now = time(0);
			returnData = myADC.measureSensor(); // Grab the current ADC measurements
			fwsp = returnData.L0[0]; // Potentiometers are connected to pins 1 & 2 of the ADC (logical 0 and 1)
			rvsp = returnData.L0[1];
			v_meas = returnData.L0[2];
			bcm2835_pwm_set_data(REV_PWM_CHANNEL, 0);
			bcm2835_pwm_set_data(FWD_PWM_CHANNEL, 0);
//			if (fwsp >= 30)
//			{
//				//			motorForward();
//				bcm2835_pwm_set_data(FWD_PWM_CHANNEL, fwsp);
//				if (i % 800 == 0) // Let's give diagnostics.
//				{
//					cout << string(100, '\n') << "Forward: " << fwsp << " | Reverse: " << rvsp << endl;
//					i = 0;
//				}
//				i++;
//			}
//	
//			if (rvsp >= 30)
//			{
//				//			motorReverse();
//				bcm2835_pwm_set_data(REV_PWM_CHANNEL, rvsp);
//				if (i % 800 == 0) // Let's give diagnostics.
//				{
//					cout << string(100, '\n') << "Forward: " << fwsp << " | Reverse: " << rvsp << endl;
//					i = 0;
//				}
//				i++;
//			}
			if ((now - tlast) == 1) // This if block controls the logging, log every second at present
			{
				tlast = now;
				v_pred = v_meas*3.3*0.005*1.20021797; // multiply by 3.3 and by 0.005 to convert to panel volts, last number is correction for error
				logData(v_meas,v_pred);
			}
		}

	return 0;
}

