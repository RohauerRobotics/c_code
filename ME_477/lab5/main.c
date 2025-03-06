/*
 * Lab #5
 * Author: Dylan Rohauer
 * Date: 3/5/25
 * Description: This lab focuses on implementing multi-threading to
 * handle an interrupt service request triggered by an external input on DIO 1.
 */

/* includes */
#include <stdio.h>
#include <stdint.h>
#include "MyRio.h"
#include "MyRio1900.h"
#include "NiFpga_MyRio1900Fpga60.h"
#include "T1.h"
#include "DIIRQ.h"
#include <pthread.h>

/* prototypes */
void wait(void);
/* definitions */

/* Global Structures and Variables */

// Thread Resource struct
typedef struct {
	NiFpga_IrqContext irqContext; // count of thread interactions?
	NiFpga_Bool irqThreadReady; // bool of p-thread status
	uint8_t irqNumber; // number of interrupts called
} ThreadResource;

// Define pthread that will handle the IRQ
// digital interrupt interrupt service routine
void* DI_ISR(void *thread_resource){
	// cast thread pointer to actual object of ThreadResource type
	ThreadResource *ISR_Resource = (ThreadResource*) thread_resource;

	uint32_t irqAssert;
	// maintain thread operations until flag appears
	while(ISR_Resource->irqThreadReady==NiFpga_True){
		// wait until an interrupt is detected
		irqAssert = 0;
		Irq_Wait(ISR_Resource->irqContext,
				ISR_Resource->irqNumber,
				&irqAssert,
				(NiFpga_Bool*)&(ISR_Resource->irqThreadReady));
		// if the interrupt is triggered by the DI0 pin
		if (irqAssert & (1<<ISR_Resource->irqNumber)){
			printf("IRQ Assert Number: %d\n", irqAssert);
//			printf("IRQ Number: %b\n",1<<ISR_Resource->irqNumber);
			printf_lcd("\ninterrupt_");
			Irq_Acknowledge(irqAssert);
		}
	}
	pthread_exit(NULL);
	return NULL;
}

// 5 ms blocking wait() function
void wait(void){
	uint32_t count = 417000;
	while(count>0){
		count--;
	}
}

int main(int argc, char **argv)
{
	NiFpga_Status status;

    status = MyRio_Open();		    			// open FPGA session
    if (MyRio_IsNotSuccess(status)) return status;

    // setup digital interrupt
    int32_t irq_status;
    uint8_t irq_number = 2;

    //IRQ Channel Settings
    MyRio_IrqDi irqDI0;
    // Note that the definitions of the IRQ adresses provided were not the same as
    // the ones in the book
    irqDI0.dioCount          = NiFpga_MyRio1900Fpga60_ControlU32_IRQDIO_A_0CNT;
    irqDI0.dioIrqNumber      = NiFpga_MyRio1900Fpga60_ControlU8_IRQDIO_A_0NO;
    irqDI0.dioIrqEnable      = NiFpga_MyRio1900Fpga60_ControlU8_IRQDIO_A_70ENA;
    irqDI0.dioIrqRisingEdge  = NiFpga_MyRio1900Fpga60_ControlU8_IRQDIO_A_70RISE;
    irqDI0.dioIrqFallingEdge = NiFpga_MyRio1900Fpga60_ControlU8_IRQDIO_A_70FALL;
    irqDI0.dioChannel        = Irq_Dio_A0;

    // Declare thread resource and set the IRQ number
    ThreadResource irqThread0;
    irqThread0.irqNumber = irq_number;

    // Register DIO interrupt service request
    irq_status = Irq_RegisterDiIrq(&irqDI0,
    		&(irqThread0.irqContext),
    		irq_number,
    		1,
    		Irq_Dio_FallingEdge);
    // falling edge indicates where a signal might change from
    // high to low

    printf("IRQ Status (Registering IRQ): %d \n", irq_status);
    // Set the ready flag to enable new thread
    irqThread0.irqThreadReady = NiFpga_True;

    // Create new thread to "catch" IRQ
    pthread_t thread;
    irq_status = pthread_create(&thread,
    							NULL,
    							DI_ISR,
    							&irqThread0);

    printf("IRQ Status (Starting POSIX Thread with thread resource): %d \n", irq_status);

    // ----------- Other main() tasks here ------------
    uint8_t loop_count = 0;
    uint8_t i; // sub loop count
    for(loop_count =1;loop_count<60; loop_count++){
    	// wait for 1 second by calling wait 200
    	for(i=0;i<200;i++){
    		wait();
    	}
    	printf_lcd("\fCount is: %d",loop_count);
    }
    // end thread
    irqThread0.irqThreadReady = NiFpga_False;
    irq_status = pthread_join(thread, NULL);
    irq_status = Irq_UnregisterDiIrq(&irqDI0,
    		irqThread0.irqContext,
    		irq_number);


	status = MyRio_Close();						// close FPGA session

	return status;
}
