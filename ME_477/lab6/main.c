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
#include "matlabfiles.h"
#include <pthread.h>

// saturation function
#define SATURATE(x,lo,hi) ((x)<(lo)?(lo):(x)>(hi)?(hi):(x))

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
	int c_i;
	y_n = x_in;
	for(c_i=0;c_i<n_s;c_i++){
//		printf("b0: %f \n", p->b0);
//		printf("a0 %f\n", p->a0);
		p->x0 = y_n;
//		printf("x0: %f\n",p->x0);
		y_n = ((p->b0*p->x0)+(p->b1*p->x1)+(p->b2*p->x2)-(p->a1*p->y1)-(p->a2*p->y2))/(p->a0);
		// shuffle values forward
		p->x2 = p->x1;
		p->x1 = p->x0;
		p->y2 = p->y1;
		p->y1 = y_n;
		// on last run through set y1 to saturated value
		// for voltage protection
		if (c_i==n_s-1){
			p->y1 = SATURATE(y_n,y_min,y_max);
			y_n = SATURATE(y_n,y_min,y_max);
		}
		// increment biquad
		p++;
	}
	return y_n; // return the result
}

// global variables
NiFpga_Session myrio_session;
static const int32_t timeoutValue = 500;

// buffer variable
#define IMAX 1000              // max points
static  double out_buffer[IMAX];  // speed buffer
static  double *out_bp = out_buffer;  // buffer pointer
static  double in_buffer[IMAX];  // speed buffer
static  double *in_bp = in_buffer;  // buffer pointer


void* Timer_ISR(void*thread_resource){
	// cast shared resource and define irq assert
	ThreadResource *ISR_Resource = (ThreadResource*) thread_resource;
	uint32_t irqAssert;
	printf("Thread Beginning!\n");

	// define a biquad representing the discretized transfer function
	int myFilter_ns = 2; // number of biquad sections
	static struct biquad myFilter[] = {
	  {1.0000e+00,  9.9999e-01, 0.0000e+00,
	   1.0000e+00, -8.8177e-01, 0.0000e+00, 0, 0, 0, 0, 0},
	  {2.1878e-04,  4.3755e-04, 2.1878e-04,
	   1.0000e+00, -1.8674e+00, 8.8220e-01, 0, 0, 0, 0, 0}
	};
	// ---- initialize analog input and output ----
	MyRio_Aio AIC0; // input
	MyRio_Aio AOC1; // output
//	MyRio_Aio AOC2; // output
	Aio_InitCI0(&AIC0);
	Aio_InitCO1(& AOC1);

	// declare static input/output for better processing speed
	static double x_input;
	static double y_out;
	static int count=0;
	static int saved = 0;

	while(ISR_Resource->irqThreadReady==1){
		Irq_Wait(ISR_Resource->irqContext,
					TIMERIRQNO,
					&irqAssert,
					(NiFpga_Bool*) &(ISR_Resource->irqThreadReady));
		if(irqAssert & (1<<TIMERIRQNO)){
			// interrupt service code
//			printf("Timer is working!\n");
//			printf_lcd("\fTimer is working!");
			x_input = Aio_Read(&AIC0);
//			printf("x_input value: %.2f\n",x_input);

			y_out = cascade(x_input,myFilter,myFilter_ns, -10, 10);
//			printf("Output Val: %f\n", y_out);

//			printf("y output value: %.2f\n",y_out);
			Aio_Write(&AOC1, y_out);

			if (in_bp < in_buffer + IMAX){
				*in_bp++ = x_input;
				*out_bp++ = y_out;
				printf("Num Samples: %d\n", count++);
			}
			else if (in_bp==in_buffer+ IMAX&& saved==0){
			    printf("Saving Data.\n");
			    int *err;
			    MATFILE *mf;
			    mf = openmatfile("Sine_200Hz.mat", &err);
			    if(!mf) printf("Can't open mat file %d\n", err);
			    matfile_addmatrix(mf, "Input", in_buffer, IMAX, 1, 0);
			    matfile_addmatrix(mf, "Output", out_buffer, IMAX, 1, 0);
			    matfile_close(mf);
			    saved = 1;
			}

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

void wait_5ms(void){
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

    printf("Saturation Test: SAT(15,-10,10) = %d\n",SATURATE(10,-15,15));
    printf("Saturation Test: SAT(15,-10,10) = %d\n",SATURATE(-10,-15,15));
    printf("Saturation Test: SAT(15,-10,10) = %d\n",SATURATE(5,-15,15));
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
    static char c;
    static int w_i;
    while((c=getkey())!=DEL){
    	// wait 25 ms
    	for (w_i=0;w_i<3;w_i++){
    		wait_5ms();
    	}
    }

    // kill thread
    printf_lcd("\fEnded Program.");
    irqThread0.irqThreadReady = NiFpga_False;
    irq_status = pthread_join(thread, NULL);
    printf("IRQ Status: %d\n", irq_status);
    irq_status = Irq_UnregisterTimerIrq(&irqTimer0,
    						irqThread0.irqContext);
    printf("IRQ Status: %d\n", irq_status);

//    if (&in_bp!=in_buffer){
//    		printf("Saving Data.\n");
//    		int *err;
//    		MATFILE *mf;
//    		mf = openmatfile("Square_8HZ.mat", &err);
//    		if(!mf) printf("Can't open mat file %d\n", err);
////    		matfile_addstring(mf,"name","Dylan");
//    		matfile_addmatrix(mf, "Input", in_buffer, IMAX, 1, 0);
//    		matfile_addmatrix(mf, "Output", out_buffer, IMAX, 1, 0);
//    		matfile_close(mf);
//    }


	status = MyRio_Close();						// close FPGA session

	return status;
}
