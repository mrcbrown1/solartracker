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

#define UPPERLIMITPIN RPI_GPIO_P1_15 // This pin is confirmed to work
#define LOWERLIMITPIN RPI_GPIO_P1_11 // This pin is confirmed to work

// Define my global variables here

int fwsp, rvsp; // Forward and reverse speed value holders
time_t now = time(0); // New time object to hold the current time
const int tstart = time(0); // When the program was started, for logging purposes
int tlast = tstart; // This is a counter to slow logging down to n seconds between logs
int upperhit, lowerhit; // Variables to hold whether the limit of travel has been reached
int direction = 0; // 0 = forward, 1 = reverse

int bufcount = 0; // index for circular buffer
#define bufsize 10 // number of previous readings to store

int voltbuffer[bufsize] = {0}; // element array allocated for averaging
int ampbuffer[bufsize] = {0};
int lightbuffer[bufsize] = {0};

int i = 0; // int to act as counter for debugging
int v_meas = 0; // int to hold the measured voltage from the ADC 0-1024
float v_pred = 0; // float to hold decimal approximation of voltage

int lightlevel = 0;

int i_meas = 0; // int to hold the measured current from the ADC 0-1024
float i_pred = 0; // float to hold decimal approximation of the current

int v_last[4] = {0};

// This is where the log should be saved, accessible by the webserver
const string PATHTOLOG = "/var/www/html/datafiles/";
int logchecked = 0; // Flag for whether the log has been rotated
int loginitialised = 0;

mcp3008 myADC; // Create new instance of ADC object

returnVars returnData; // returnVars is defined in mcp3008.h and is just the return data structure

mcp3008Spi a2d("/dev/spidev0.0", SPI_MODE_0, 1000000, 8); // Making a global instance of the SPI interface to avoid creating/destroying too much

/*
 Functions start here
 */

float averageArray(int *inputs, int n) // This function will return the floating point average of an array of integers
{
	//	int n = sizeof (inputs);
	float sum = 0;
	float count = 0;
	for (int i = 0; i < n; i++)
	{
		int value = abs(inputs[i]);
		if (value > 0)
		{
			sum += value;
			count++;
		}
	}
	float average = sum / count;
	//		cout << "Sum: " << sum << " | Count: " << n << " | Average: " << average << endl;
	return average;
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
				rename(logpath.c_str(), path.c_str());
			}

			maxindex++;
		}
	}
}

static void logData(int voltage, int current, int light) // This function appends data to a csv file, needs to check for existing files....
{
	if (logchecked == 0)
		checkLogfile();

	if (loginitialised == 0)
	{
		string path = PATHTOLOG + "data.csv";
		ofstream file(path, std::ios::app); //open in constructor, make sure we append data
		file << "Time (s), Voltage (V), Current (I), Light Level (Lx)" << endl; // Add column headers for the data
		loginitialised = 1;
	}

	if ((now - tlast) >= 1)
	{
		tlast = now;
		now = time(0); //Get the current time
		tm *ltm = localtime(&now); // Convert it into tm struct for easy access

		int day = ltm->tm_mday; // Extract the day, month and year
		int month = ltm->tm_mon;
		int year = 1900 + ltm->tm_year;

		string path = PATHTOLOG + "data.csv";
		ofstream file(path, std::ios::app); //open in constructor, make sure we append data
		file << now - tstart << "," << voltage << "," << current << endl; // Echo in the correct data, return to main loop, destructor auto called
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

static void initialScan()
{
	int thisstart = time(0);
	while (now - thisstart <= 9)
	{
		bcm2835_pwm_set_data(FWD_PWM_CHANNEL, 256);

		now = time(0); // Get current time

		returnData = myADC.measureSensor(); // Grab the current ADC measurements
		checkLimits();

//		lightlevel = returnData.L0[0];
		v_meas = returnData.L0[1];


		voltbuffer[bufcount] = v_meas;
//		lightbuffer[bufcount] = lightlevel;


		bufcount++;
		if (bufcount == (bufsize))
		{
			bufcount = 0;
		}
		
		v_last[now - tstart] = (int) averageArray(voltbuffer, bufsize);
	}
	bcm2835_pwm_set_data(FWD_PWM_CHANNEL, 0);
}

int main()
{
	if (!bcm2835_init()) // Exit if BCM2835 isn't initialised
		return 1;
	cout << "BCM2835 Initialised!" << endl;
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

	bcm2835_pwm_set_data(FWD_PWM_CHANNEL, 0);

	int detecting = 1;
	cout << "System will attempt to move now" << endl;
	cout << "Entering initial loop" << endl;
	while (detecting)
	{
		initialScan();

		if ((int) averageArray(voltbuffer, bufsize) < v_last[1]) // If light is lower, go the in reverse
		{
			cout << "I was moving in direction " << direction;
			direction = 1;
			cout << ", I am now moving in direction " << direction << endl;
		} else if ((int) averageArray(voltbuffer, bufsize) > v_last[1]) // If light is higher, keep going
		{
			cout << "I was moving in direction " << direction;
			detecting = 0;
			cout << ", I am now moving in direction " << direction << endl;
		}
	}
	cout << "Exited initial loop" << endl;

	cout << "Voltage at start of initialising was: " << to_string(v_last[1]) << ", voltage after was " << to_string(averageArray(voltbuffer, bufsize)) << endl;
	return 0;

//	while (true)
//	{
//		//-------------------DATA EXTRACTION AND BUFFER ADMIN--------------------------//
//
//		now = time(0); // Get current time
//
//		returnData = myADC.measureSensor(); // Grab the current ADC measurements
//		checkLimits();
//
//		lightlevel = returnData.L0[0];
//		v_meas = returnData.L0[1];
//		rvsp = returnData.L0[0];
//
//		voltbuffer[bufcount] = v_meas;
//		lightbuffer[bufcount] = lightlevel;
//
//
//		bufcount++;
//		if (bufcount == (bufsize))
//		{
//			bufcount = 0;
//		}
//
//		//--------------------------------------------------------------------//
//
//
//		if (rvsp >= 30)
//		{
//			//			rvsp = (int)averageArray(voltbuffer, bufsize);
//			bcm2835_pwm_set_data(FWD_PWM_CHANNEL, rvsp);
//			bcm2835_pwm_set_data(REV_PWM_CHANNEL, rvsp);
//		}
//		if (rvsp < 30)
//		{
//			bcm2835_pwm_set_data(FWD_PWM_CHANNEL, 0);
//			bcm2835_pwm_set_data(REV_PWM_CHANNEL, 0);
//		}
//
//
//
//		v_meas = averageArray(voltbuffer, bufsize);
//		lightlevel = averageArray(lightbuffer, bufsize);
//		i_meas = averageArray(ampbuffer, bufsize);
//
//		v_pred = v_meas * 3.3 * 0.005 * 1.22850123; // multiply by 3.3 and by 0.005 to convert to panel volts, last number is correction for error
//		i_pred = i_meas;
//		//		logData(v_pred, i_pred, lightlevel);
//		cout << rvsp << endl;
//		delayMicroseconds(100000);
//	}
//
//	return 0;
}

