/*
 * Lab #6
 * Author: Dylan Rohauer
 * Date: 3/8/25
 * Description: A lab to realize a discrete dynamic control system
 * (a Butterworth filter) based on an input received from an external
 * analog input, interrupt timing, and output the result to an analog pin.
 */

/* includes */
#include <stdint.h>
#include <stdio.h>
#include "MyRio.h"
#include "T1.h"
#include "MyRio1900.h"
#include "NiFpga_MyRio1900Fpga60.h"
#include "DIIRQ.h"
#include "TimerIRQ.h"
#include <pthread.h>

// saturation function
#define SATURATE(x,lo,hi) ((x)<(lo)?(lo):(x)>(hi)?(hi):(x))
/* prototypes */

/* definitions */

// globally declared thread resource structure
typedef struct {
	NiFpga_IrqContext irqContext; // IRQ Context reserved
	NiFpga_Bool irqThreadReady; // ready flag
} ThreadResource;

// define biquad parameter structure
struct biquad {
	double b0; double b1; double b2; // numerator
	double a0; double a1; double a2; // denominator
	double x0; double x1; double x2; // input
	double y1; double y2; // output
};

// define cascade function
double cascade(double x_in, struct biquad *fa, int n_s, double y_min, double y_max){
	struct biquad *p;
	p = fa;
	double y_n;
	int i;
	for(i=0;i<n_s;i++){
		y_n = ((p->b0*x_in)+(p->b1*p->x1)+(p->b2*p->x2)-(p->a1*p->y1)-(p->a2*p->y2))/(p->a0);
		// shuffle values forward
		p->x2 = p->x1;
		p->x1 = x_in;
		p->y2 = p->y1;
		p->y1 = y_n;
		// increment biquad
		p++;
	}
	return y_n; // return the result
}

// global variables
NiFpga_Session myrio_session;
int32_t timeoutValue = 500000;


void* Timer_ISR(void*thread_resource){
	// cast shared resource and define irq assert
	ThreadResource *ISR_Resource = (ThreadResource*) thread_resource;
	uint32_t irqAssert;
	printf("Thread Beginning!\n");

	// define a biquad representing the discretized transfer function
	int myFilter_ns = 2; // number of biquad sections
	static struct biquad myFilter[] = {
			{1.000e+00,9.999e-01,0.0000e00,1.000e+00,-9.8177e-01,0.0000e+00,0,0,0,0,0},
			{2.1878e-04,4.3755e-04,2.1878e-04,1.0000e+00,-1.8674e+00,8.8220e-01,0,0,0,0,0}
	};
	while(ISR_Resource->irqThreadReady==1){
		Irq_Wait(ISR_Resource->irqContext,
					TIMERIRQNO,
					&irqAssert,
					(NiFpga_Bool*) &(ISR_Resource->irqThreadReady));
		if(irqAssert & (1<<TIMERIRQNO)){
			// interrupt service code
			printf("Timer is working!\n");
			printf_lcd("\fTimer is working!");

			// set next timer
			NiFpga_WriteU32(myrio_session, NiFpga_MyRio1900Fpga60_ControlU32_IRQTIMERWRITE,timeoutValue);
			NiFpga_WriteBool(myrio_session, NiFpga_MyRio1900Fpga60_ControlBool_IRQTIMERSETTIME,NiFpga_True);
			Irq_Acknowledge(irqAssert);
		}
		else{
			printf("No Timer Activation.\n");
		}
	}

	// end thread after flag
	pthread_exit(NULL);
	return NULL;
}
int main(int argc, char **argv)
{
	NiFpga_Status status;

    status = MyRio_Open();		    			// open FPGA session
    if (MyRio_IsNotSuccess(status)) return status;

    // Timer Interrupt Setup
    int32_t irq_status;
    MyRio_IrqTimer irqTimer0;
    ThreadResource irqThread0;
    pthread_t thread;

    irqTimer0.timerWrite = NiFpga_MyRio1900Fpga60_ControlU32_IRQTIMERWRITE;
    irqTimer0.timerSet = NiFpga_MyRio1900Fpga60_ControlBool_IRQTIMERSETTIME;

    irq_status = Irq_RegisterTimerIrq(&irqTimer0,
    								&irqThread0.irqContext,
    								timeoutValue);
    irqThread0.irqThreadReady = NiFpga_True;

    printf("Register Timer IRQ: %d\n",irq_status);
    irq_status = pthread_create(&thread,
    							NULL,
    							Timer_ISR,
    							&irqThread0);

    // ------ other main() stuff ------
    char c;
    while((c=getkey())!=DEL){
    	// main loop stuff
    }

    // kill thread
    irqThread0.irqThreadReady = NiFpga_False;
    irq_status = pthread_join(thread, NULL);
    printf("IRQ Status: %d\n", irq_status);
    irq_status = Irq_UnregisterTimerIrq(&irqTimer0,
    						irqThread0.irqContext);
    printf("IRQ Status: %d\n", irq_status);


	status = MyRio_Close();						// close FPGA session

	return status;
}
