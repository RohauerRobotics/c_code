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

/* prototypes */

/* definitions */
#define PRINT_PIN 1
#define STOP_PIN 3
#define PWM_PIN 5

// define enumerated states
// assigns number values to each state
//typedef enum {
//	HIGH_S = 0,
//	LOW_S,
//	SPEED_S,
//	STOP_S,
//	EXIT_S
//} State_Type;

static MyRio_Dio init_array[3];
static int init_index[3] = {PRINT_PIN,STOP_PIN,PWM_PIN};
NiFpga_Session myrio_session;
MyRio_Encoder encC0;

void initSM(void){
	// initialize DIO channel C pins 1,3,5 for printS, PWM signal, and stopS
	int i;
	for (i=0; i<3;i++){
		init_array[i].dir = DIOC_70DIR;   // "70" used for DIO 0-7
		init_array[i].out = DIOC_70OUT;   // "70" used for DIO 0-7
		init_array[i].in  = DIOC_70IN;    // "70" used for DIO 0-7
		init_array[i].bit = init_index[i];
	}

	EncoderC_initialize(myrio_session, &encC0);
}

void test_motor(void){
	time_t start_time, end_time;
	start_time = time(NULL);
	printf("Start Time: %.2f\n",start_time);
	// Code to be timed
	int i;
	for (i = 0; i < 100000000; i++);
	end_time = time(NULL);
	double elapsed_time = difftime(end_time, start_time);
	printf("Elapsed time: %.2f seconds\n", elapsed_time);


}

int main(int argc, char **argv)
{
	NiFpga_Status status;

    status = MyRio_Open();		    			// open FPGA session
    if (MyRio_IsNotSuccess(status)) return status;
    initSM();
    test_motor();
    //my code here
    printf("Hello World!\n");				// Print to console
    printf_lcd("\fHello World!\n");		// Print to LCD display

	status = MyRio_Close();						// close FPGA session

	return status;
}
