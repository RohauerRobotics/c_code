/*
 * Lab #4
 * Author: Dylan Rohauer
 * Date: 2/26/25
 * Description: Lab to operate motor
 */

/* includes */
#include <stdio.h>
#include "Encoder.h"
#include "MyRio.h"
#include "DIO.h"
#include "T1.h"
#include "matlabfiles.h"
//#include <time.h>
//#include "emulate.h"
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
const double wait_dur = (1.0/667.0)*0.000001*834017.0; // wait delay in seconds


// declare button pins
static MyRio_Dio pin_array[3];
static int init_index[3] = {PRINT_PIN,PWM_PIN,STOP_PIN};

// define myRio session for encoder
NiFpga_Session myrio_session;
MyRio_Encoder encC0;
//encC0.cnfg = ENCC_OCNFG;
//encC0.stat = ENCC_OSTAT;
//encC0.cntr = ENCC_OCNTR;

// define enumerated states
// assigns number values to each state
typedef enum {
	HIGH_S = 0,
	LOW_S,
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
//static int buffer; // declare buffer list for plotting data

// declare user inputs
static int N; // number of low motor periods in a BTI
static int M; // number of high motor periods in a BTI

// velocity variable
//static double vel;

// counter variable
static uint32_t prev_count;
static uint32_t curr_count;

// define low() state function
void low(void){
	if (clk==N){
//		printf("Low State Complete.\n");
		clk = 0;
		run = 1;
		curr_state = HIGH_S;
	}
	else if (print_b==1){
		curr_state = SPEED_S;
		print_b = 0;
	}
	else if (stop_b ==1){
		curr_state = STOP_S;
	}
}

// define high() state function
void high(void){
	if (clk==M){
//		printf("High State Complete.\n");
		run = 0;
		curr_state = LOW_S;
	}
}

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

void speed(void){
	double increment_velocity = velocity();
	printf("Increment Vel: %.2f (increments/second)\n", increment_velocity);
	// RPM = (increments/sec)*(seconds/minute)*(revolutions/increment)
	double RPM = increment_velocity*(60.0)*(1.0/2048.0);
	printf_lcd("\fSpeed: %f RPM",RPM);
	printf("RPM: %.2f (rotations/minute)\n",RPM);
	curr_state = LOW_S;
}

void stop(void){
	run = 0;
	printf_lcd("\fStopping...");
	curr_state = EXIT_S;
}

void exit_func(void){
	printf("Stopped the program.\n");
}

void initSM(void){
	// initialize DIO channel C pins 1,3,5 for printS, PWM signal, and stopS
	int i;
	for (i=0; i<3;i++){
		pin_array[i].dir = DIOC_70DIR;   // "70" used for DIO 0-7
		pin_array[i].out = DIOC_70OUT;   // "70" used for DIO 0-7
		pin_array[i].in  = DIOC_70IN;    // "70" used for DIO 0-7
		pin_array[i].bit = init_index[i];
	}
	EncoderC_initialize(myrio_session, &encC0);
	prev_count = Encoder_Counter(&encC0); // set initial counter
	printf("Encoder Value: %d\n", prev_count);
	// write PWM Low
	Dio_WriteBit(&pin_array[PWM_INDEX],0);
	curr_state = LOW_S;
	clk = 0;
}


void wait(void){
	uint32_t count_wait = 417000;
	while (count_wait>0){
		count_wait--;
	}
	clk++;
}

static void (*state_table[])(void) ={
	high,low,speed,stop,exit_func
};

int main(int argc, char **argv)
{
	NiFpga_Status status;

    status = MyRio_Open();		    			// open FPGA session
    if (MyRio_IsNotSuccess(status)) return status;

    // initialize state machine
    initSM();
    printf("Wait Duration is: %f seconds\n",wait_dur);
    N = double_in("\fBTI Intervals: ");
    printf("Received N intervals: %d\n",N);
    curr_count = Encoder_Counter(&encC0);
    printf("New Encoder Count: %d\n", curr_count);
    prev_count = curr_count;
    M = double_in("\fOn Intervals:");
    printf("Received M intervals: %d\n",M);
    while (curr_state!=EXIT_S){
//    	printf("Run Value: %d\n", run);
    	Dio_WriteBit(&pin_array[PWM_INDEX],run);
    	print_b = Dio_ReadBit(&pin_array[PRINT_INDEX]);
    	//    	printf("Print Button: %d\n", print_b);
    	stop_b = Dio_ReadBit(&pin_array[STOP_INDEX]);
    	//    	printf("Stop Button: %d\n", stop_b);
    	state_table[curr_state]();
    	wait();
    }

	status = MyRio_Close();						// close FPGA session

	return status;
}
