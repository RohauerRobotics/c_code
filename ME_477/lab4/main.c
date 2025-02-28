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
#include <time.h>
//#include "emulate.h"
#include "MyRio1900.h"
#include <stdint.h>

/* prototypes */

/* definitions */
#define PRINT_PIN 1
#define PRINT_INDEX 0

#define STOP_PIN 3
#define STOP_INDEX 1

#define PWM_PIN 5
#define PWM_INDEX 2

// declare button pins
static MyRio_Dio pin_array[3];
static int init_index[3] = {PRINT_PIN,STOP_PIN,PWM_PIN};

// define myRio session for encoder
NiFpga_Session myrio_session;
MyRio_Encoder encC0;

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

// declare user inputs
static int N; // number of low motor periods in a BTI
static int M; // number of high motor periods in a BTI

// define low() state function
void low(void){
	if (clk==N){
		clk = 0;
		run = 1;
	}
	else if (print_b==1){
		curr_state = SPEED_S;
	}
}

// define high() state function
void high(void){
	if (clk==M){
		run = 0;
		curr_state = LOW_S;
	}
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
	Dio_WriteBit(&pin_array[PWM_INDEX],0);
}


void wait(void){
	uint32_t count_wait = 417000;
	while (count_wait>0){
		count_wait--;
	}
}


void check_print(void){
	print_b = Dio_ReadBit(&pin_array[PRINT_INDEX]);
}

void check_stop(void){
	stop_b = Dio_ReadBit(&pin_array[STOP_INDEX]);
}

int main(int argc, char **argv)
{
	NiFpga_Status status;

    status = MyRio_Open();		    			// open FPGA session
    if (MyRio_IsNotSuccess(status)) return status;

    // initialize state machine
    initSM();

	status = MyRio_Close();						// close FPGA session

	return status;
}
