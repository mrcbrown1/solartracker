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

// Define a few constants here

#define MOTORFORWARDPIN RPI_V2_GPIO_P1_33
#define MOTORREVERSEPIN RPI_GPIO_P1_12
#define FWD_PWM_CHANNEL 1
#define REV_PWM_CHANNEL 0
#define RANGE 1024

#define UPPERLIMITPIN RPI_GPIO_P1_13
#define LOWERLIMITPIN RPI_GPIO_P1_13

// Define my global variables here

int fwsp, rvsp;
time_t now = time(0); // New time object to hold the current time
const int tstart = time(0); // When the program was started, for logging purposes
int tlast = tstart; // This is a counter to slow logging down to n seconds between logs
int upperhit, lowerhit; // Variables to hold whether the limit of travel has been reached

// This is where the log should be saved, accessible by the webserver
const string PATHTOLOG = "/var/www/html/datafiles/";
int logchecked = 0; // Flag for whether the log has been rotated

mcp3008 myADC; // Create new instance of ADC object

returnVars returnData; // returnVars is defined in mcp3008.h and is just the return data structure

mcp3008Spi a2d("/dev/spidev0.0", SPI_MODE_0, 1000000, 8); // Making a global instance of the SPI interface to avoid creating/destroying too much

/*
 Functions start here
 */

float averageArray(int inputs[]) // This function will return the floating point average of an array of integers
{
	int n = sizeof (inputs);
	int sum = 0;
	int count = 0;
	for (int i = 0; i < n; i++)
	{
		if (inputs[i] != 0)
		{
			sum += inputs[i];
			count++;
		}
	}
	float average = sum / count;
	//	cout << "Sum: " << sum << " | Count: " << n << " | Average: " << average << endl;
	return average;
}

static void logData(int voltage, float current) // This function appends data to a csv file, needs to check for existing files....
{
	if (logchecked == 0)
		checkLogfile();
	
	if ((now - tlast) >= 1)
	{
		tlast = now;
		now = time(0); //Get the current time
		tm *ltm = localtime(&now); // Convert it into tm struct for easy access

		int day = ltm->tm_mday; // Extract the day, month and year
		int month = ltm->tm_mon;
		int year = 1900 + ltm->tm_year;

		string varname = to_string(day) + "-" + to_string(month) + "-" + to_string(year); // Make a unique filename for the log
		string path = PATHTOLOG + "data.csv";
		ofstream file(path, std::ios::app); //open in constructor, make sure we append data
		file << now - tstart << "," << voltage << "," << current << endl; // Echo in the correct data, return to main loop, destructor auto called
	}
}

static void checkLogfile()
{
	int maxindex = 0;
	int maxfound = 0;
	logchecked = 1;
	
	string logpath = PATHTOLOG + "data.csv";

	if (std::ifstream(logpath))
	{
		while (maxfound == 0)
		{
			string path = PATHTOLOG + "data" + to_string(maxindex + 1) + ".csv";

			if (!std::ifstream(path))
			{
				maxfound = 1;
				rename(logpath, path);
			}

			maxindex++;
		}
	}
}

static void checkLimits()
{
	uint8_t upper = bcm2835_gpio_lev(UPPERLIMITPIN);
	uint8_t lower = bcm2835_gpio_lev(LOWERLIMITPIN);

	if (upper == 0)
	{
		upperhit = 1;
	} else
	{
		upperhit = 0;
	}
	if (lower == 0)
	{
		lowerhit = 1;
	} else
	{
		lowerhit = 0;
	}
}

int main()
{
	if (!bcm2835_init()) // Exit if BCM2835 isn't initialised
		return 1;

	//--------------SETUP PWM HERE----------------//

	bcm2835_gpio_fsel(MOTORFORWARDPIN, BCM2835_GPIO_FSEL_ALT0); // Make forward and reverse pins connected to respective PWM clocks.
	bcm2835_gpio_fsel(MOTORREVERSEPIN, BCM2835_GPIO_FSEL_ALT5); // ALT5 is PWM channel 0, and ALT0 is PWM channel 1
	bcm2835_pwm_set_clock(BCM2835_PWM_CLOCK_DIVIDER_256); //Set clock divider : 1,2,4,8,16,32,64,128,256,512,1024,2048

	bcm2835_pwm_set_mode(FWD_PWM_CHANNEL, 1, 1); // Set mode and range for both forward and reverse channels
	bcm2835_pwm_set_range(FWD_PWM_CHANNEL, RANGE); // Format is (channel, mode, enabled) mode 1 = mark-space, 2 = balanced
	bcm2835_pwm_set_data(FWD_PWM_CHANNEL, 0); // Reset PWM channel to zero incase there was leftover data

	bcm2835_pwm_set_mode(REV_PWM_CHANNEL, 1, 1); // These are the same as above
	bcm2835_pwm_set_range(REV_PWM_CHANNEL, RANGE); // Channel is either 0 or 1, enabled is self explanatory
	bcm2835_pwm_set_data(REV_PWM_CHANNEL, 0); // Reset PWM channel to zero incase there was leftover data

	//--------------SETUP LIMIT SWITCHES HERE----------------//

	bcm2835_gpio_fsel(UPPERLIMITPIN, BCM2835_GPIO_FSEL_INPT); // Set the limit switches to be inputs
	bcm2835_gpio_fsel(LOWERLIMITPIN, BCM2835_GPIO_FSEL_INPT);

	bcm2835_gpio_set_pud(UPPERLIMITPIN, BCM2835_GPIO_PUD_UP); // Set the limit switches to have a pull up resistor
	bcm2835_gpio_set_pud(LOWERLIMITPIN, BCM2835_GPIO_PUD_UP);

	//---------------START OF MAIN LOOP---------//

	int i = 0; // int to act as counter for debugging
	int v_meas = 0; // int to hold the measured voltage from the ADC 0-1024
	float v_pred = 0; // float to hold decimal approximation of voltage

	int bufcount = 0; // index for circular buffer
	int bufsize = 10; // number of previous readings to store
	int buffer[bufsize] = {0}; // element array allocated for averaging
	int buffer2[bufsize] = {0};


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

		buffer2[bufcount] = rvsp;
		buffer[bufcount] = v_meas;
		bufcount++;
		if (bufcount == (bufsize))
		{
			bufcount = 0;
		}

		if (rvsp >= 30)
		{
			//			motorReverse();
			rvsp = averageArray(buffer2);
			bcm2835_pwm_set_data(REV_PWM_CHANNEL, rvsp);
			if ((now - tlast) >= 1) // This if block controls the logging, log every second at present
			{
				tlast = now;
				v_meas = averageArray(buffer);
				v_pred = v_meas * 3.3 * 0.005 * 1.20021797; // multiply by 3.3 and by 0.005 to	 convert to panel volts, last number is correction for error
				logData(v_meas, v_pred);
			}

		}


		if ((now - tlast) >= 1) // This if block controls the logging, log every second at present
		{
			tlast = now;
			v_meas = averageArray(buffer);
			v_pred = v_meas * 3.3 * 0.005 * 1.20021797; // multiply by 3.3 and by 0.005 to convert to panel volts, last number is correction for error
			logData(v_meas, v_pred);
			//				cout << averageArray(buffer) << endl;
			//				cout << endl;
		}
	}

	return 0;
}

