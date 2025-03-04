/*
 * Lab #4
 * Author: Dylan Rohauer
 * Date: 3/3/25
 * Description: This lab has us make a finite state machine
 * with the goal of operating a DC motor via open loop control.
 */

/* includes */
#include <stdio.h>
#include "Encoder.h"
#include "MyRio.h"
#include "DIO.h"
#include "T1.h"
#include "matlabfiles.h"
#include "MyRio1900.h"
#include <stdint.h>
#include <math.h>

/* prototypes */
void wait(void);

/* definitions */
#define PRINT_PIN 1
#define PRINT_INDEX 0

#define STOP_PIN 5
#define STOP_INDEX 2

#define PWM_PIN 3
#define PWM_INDEX 1

// define approximate wait time interval
const double wait_dur = (1.0/667.0)*0.000001*3336011.0; // wait delay in seconds


// declare button pins
static MyRio_Dio pin_array[3];
static int init_index[3] = {PRINT_PIN,PWM_PIN,STOP_PIN};

// define myRio session for encoder
NiFpga_Session myrio_session;
MyRio_Encoder encC0;

// define enumerated states
// assigns number values to each state
typedef enum {
	LOW_S = 0,
	HIGH_S,
	SPEED_S,
	STOP_S,
	EXIT_S
} State_Type;

// declare current state as State_Type type variable
static State_Type curr_state;

// define button inputs as static integers
static int print_b = 0;
static int stop_b = 0;

// declare inputs
static int clk;

// declare outputs
static int run; // PWM output state
#define IMAX 200              // max points
static  double buffer[IMAX];  // speed buffer
static  double *bp = buffer;  // buffer pointer

// declare user inputs
static int N; // number of low motor periods in a BTI
static int M; // number of high motor periods in a BTI

// speed iterations count --> for button bouncing
static int speed_count;

// encoder counter variable
static uint32_t prev_count;
static uint32_t curr_count;

// define low() state function
void low(void){
	if (clk==N){
		if (stop_b ==1){
			curr_state = STOP_S;
		}
		else if (print_b==1){
			curr_state = SPEED_S;
			print_b = 0;
			clk = 0;
			run = 1;
		}
		else{
			clk = 0;
			run = 1;
			curr_state = HIGH_S;
		}
	}
}

// define high() state function
void high(void){
	if (clk==M){
//		printf("High State Complete.\n");
		run = 0;
		curr_state = LOW_S;
		clk=0;
	}
}

// calculates increments/sec of motor
double velocity(void){
	double vel;
	// get current count
	curr_count = Encoder_Counter(&encC0);
	// get calculate velocity from wait time and number of cycles in a BTI
	vel = (curr_count - prev_count)/((N+M)*wait_dur);
	// assign prev_count to current count
	prev_count = curr_count;
	return vel;
}

// collects encoder data and calculates RPM
void speed(void){
	speed_count++; // for button bounce detection

	double increment_velocity = velocity();
	printf("Increment Vel: %.2f (increments/second)\n", increment_velocity);

	double RPM = increment_velocity*(60.0)*(1.0/2048.0);
	printf_lcd("\fSpeed: %f RPM",RPM);
	printf("RPM: %.2f (rotations/minute)\n",RPM);

	// button bounce counter
	printf("Speed Iter Count: %d\n",speed_count);

	// add RPM to buffer list
	if (bp < buffer + IMAX){
		*bp++ = RPM;
	}
	curr_state = HIGH_S;
	print_b = 0;
}

// sets run to zero and exits program
void stop(void){
	run = 0;
	printf_lcd("\fStopping...");
	curr_state = EXIT_S;
}

// final state, saves data and exits program
void exit_func(void){
	// save data to file
	if (&bp!=buffer){
		printf("Saving Data.\n");
		int *err;
		MATFILE *mf;
		mf = openmatfile("KDJD.mat", &err);
		if(!mf) printf("Can't open mat file %d\n", err);
		matfile_addstring(mf,"name","Dylan");
		matfile_addmatrix(mf, "Velocity", buffer, IMAX, 1, 0);
		matfile_close(mf);
	}
	printf("Stopped the program.\n");
}


// initializes the state machine
void initSM(void){
	// initialize DIO channel C pins 1,3,5 for printS, PWM signal, and stopS
	int i;
	for (i=0; i<3;i++){
		pin_array[i].dir = DIOC_70DIR;   // "70" used for DIO 0-7
		pin_array[i].out = DIOC_70OUT;   // "70" used for DIO 0-7
		pin_array[i].in  = DIOC_70IN;    // "70" used for DIO 0-7
		pin_array[i].bit = init_index[i];
	}
	// encoder initialization
	EncoderC_initialize(myrio_session, &encC0);
	// get starting encoder value
	prev_count = Encoder_Counter(&encC0); // set initial counter
	printf("Encoder Value: %d\n", prev_count);

	// write PWM Low
	run = 0;
	Dio_WriteBit(&pin_array[PWM_INDEX],run);

	// set low as starting state
	curr_state = LOW_S;
	clk = 0;
}

// waits for a fixed period of time
void wait(void){
	uint32_t count_wait = 417000;
	while (count_wait>0){
		count_wait--;
	}
}

// function pointer array
static void (*state_table[])(void) ={
	low,high,speed,stop,exit_func
};

int main(int argc, char **argv)
{
	NiFpga_Status status;

    status = MyRio_Open();		    			// open FPGA session
    if (MyRio_IsNotSuccess(status)) return status;

    // initialize state machine
    initSM();

    // collect static inputs
    printf("Wait Duration is: %f seconds\n",wait_dur);
    N = double_in("\fBTI Intervals: ");
    printf("Received N intervals: %d\n",N);

    // encoder start value
    curr_count = Encoder_Counter(&encC0);
    printf("New Encoder Count: %d\n", curr_count);

    prev_count = curr_count;
    M = double_in("\fOn Intervals:");
    printf("Received M intervals: %d\n",M);
    while (curr_state!=EXIT_S){
    	// collect button inputs
    	print_b = Dio_ReadBit(&pin_array[PRINT_INDEX]);
    	stop_b = Dio_ReadBit(&pin_array[STOP_INDEX]);
    	// run function pointer array
    	state_table[curr_state]();
    	// actuate PWM Output
    	Dio_WriteBit(&pin_array[PWM_INDEX],run);
//    	printf("State: %d Clock: %d Run: %d\n",curr_state, clk, run);
    	wait();
    	clk++;
    }
    state_table[curr_state]();
	status = MyRio_Close();						// close FPGA session

	return status;
}
