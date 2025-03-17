/*
 * Lab # 7
 * Author: Dylan Rohauer
 * Date: 3/16/25 (Happy Pi Day!)
 * Description: This is a lab designed to implement a motor speed controller
 * using a digitally implemented proportional - integral controller.
 */

// standard libraries
#include <stdint.h>
#include <stdio.h>
#include "MyRio.h"
#include "T1.h"

// table library
#include "ctable2.h"

// matlab data tracking
#include "matlabfiles.h"

// timer ISR and thread libraries
#include "MyRio1900.h"
#include "NiFpga_MyRio1900Fpga60.h"
#include "TimerIRQ.h"
#include <pthread.h>

// encoder libraries
#include "Encoder.h"
#include <math.h>


// --- global variables ---

// timer delay time
static int32_t timeout_value = 5000; // 5000 microseconds = 5 ms
static double val_kp = 0.104;
static double val_ki = 2.07;
static double val_vref = -200;
// timer ISR and encoder globals
NiFpga_Session myrio_session;
MyRio_Encoder my_encoder;

static int32_t prev_count;
static int32_t curr_count;

// --- global structs ---

// globally declared thread resource structure
typedef struct {
	NiFpga_IrqContext irqContext; // IRQ context reserved
	NiFpga_Bool irqThreadReady; // ready flag
	table *p_table; // ctable2 pointer
} ThreadResource;

// define bi-quad parameter structure
struct biquad {
	double b0; double b1; double b2; // numerator
	double a0; double a1; double a2; // denominator
	double x0; double x1; double x2; // input
	double y1; double y2; // output
};

// declare buffer index
# define BMAX 2500
static double meas_vel[BMAX];
static double *m_v = meas_vel;
static double out_volt[BMAX];
static double *o_v = out_volt;
static double ref_vel;
static double last_ref;

// --- functions ---

// saturation function
#define SATURATE(x,lo,hi) ((x)<(lo)?(lo):(x)>(hi)?(hi):(x))

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

// calculates increments/sec of motor
double velocity(void){
	double vel;
	// get current count
	curr_count = Encoder_Counter(&my_encoder);
//	printf("Current Count: %d\n", curr_count);
	// get calculate velocity from wait time and number of cycles in a BTI
	vel = (curr_count - prev_count)/(timeout_value*0.000001);
//	printf("Velocity A: %f\n", vel);
	vel =  vel*(60.0)*(1.0/2048.0); // convert to RPMs
//	printf("velocity B: %f\n",vel);
	// assign prev_count to current count
	prev_count = curr_count;

	return vel;
}

void* ISR_TIMER(void*thread_resource){
	printf("Thread Beginning!\n");

	// cast shared resource and define irq assert
	ThreadResource *ISR_Resource = (ThreadResource*) thread_resource;
	uint32_t irqAssert;

	// define constant pointers --> dereference to read values
	double *ref_rpm = &((ISR_Resource->p_table+0)->value); // ref rpm is the first item
	// Display values
	double *rpm_measured = &((ISR_Resource->p_table+1)->value); // ref rpm is the first item
	double *vda_out = &((ISR_Resource->p_table+2)->value); // ref rpm is the first item
	double *K_P = &((ISR_Resource->p_table+3)->value); // proportional constant
	double *K_I = &((ISR_Resource->p_table+4)->value); // integral constant
	double *BTI = &((ISR_Resource->p_table+5)->value); // integral constant

	// analog output - pin 0
	MyRio_Aio analog_out; // output
	Aio_InitCO0(&analog_out);

	// write motor low
	Aio_Write(&analog_out, 0);

	// test encoder
	EncoderC_initialize(myrio_session, &my_encoder);
	curr_count = Encoder_Counter(&my_encoder);
	printf("New Encoder Count: %d\n", curr_count);
	prev_count = curr_count;

	// define a biquad representing the discretized transfer function
	int control_ns = 1; // number of biquad sections
	static struct biquad control_coeffs[] = {
	  {1.0000e+00,  0, 0,
	   1.0000e+00, -1.0000e+00, 0,
	   0, 0, 0,
	   0, 0},
	};
	struct biquad *cc_p = control_coeffs;
	// update biquad with user inputs (if any)
	printf("BTI is: %f\n", (*BTI));
	cc_p->b0 = (*K_P) + 0.5*(*K_I)*(*BTI)*0.001;
	cc_p->b1 = -(*K_P) + 0.5*(*K_I)*(*BTI)*0.001;

	// declare current, error, and analog_out value
	static double curr_rpm;
	// define current error
	static double rpm_error;
	static double analog_value;
	double last_kp = *K_P;
	double last_ki = *K_I;
	static int iter_counts = 0;
	static int saved = 0;
	while(ISR_Resource->irqThreadReady==1){
		Irq_Wait(ISR_Resource->irqContext,
					TIMERIRQNO,
					&irqAssert,
					(NiFpga_Bool*) &(ISR_Resource->irqThreadReady));
		if(irqAssert & (1<<TIMERIRQNO)){
			if (last_ki!=(*K_I) || last_kp!=(*K_P)){
							cc_p->b0 = (*K_P) + 0.5*(*K_I)*(*BTI)*0.001;
							cc_p->b1 = -(*K_P) + 0.5*(*K_I)*(*BTI)*0.001;
							last_kp = *K_P;
							last_ki = *K_I;
			}
			curr_rpm = velocity();
			rpm_error = (*ref_rpm) - curr_rpm;
//			printf("Ref RPM: %f\n",(*ref_rpm));
//			printf("RPM Error: %f\n", rpm_error);
			analog_value = cascade(rpm_error,control_coeffs,control_ns, -10, 10);
//			printf("Output Val: %f\n", analog_value);

//			printf("y output value: %.2f\n",y_out);
			Aio_Write(&analog_out,analog_value);

			if (m_v < meas_vel + BMAX){
				*m_v++ = curr_rpm;
				*o_v++ = analog_value;
				iter_counts++;
			}
			else if (m_v==meas_vel+ BMAX&& saved==0){
			    printf("Saving Data.\n");
			    int *err;
			    MATFILE *mf;
			    mf = openmatfile("StepChange.mat", &err);
			    if(!mf) printf("Can't open mat file %d\n", err);
			    matfile_addmatrix(mf, "Velocity", meas_vel, BMAX, 1, 0);
			    matfile_addmatrix(mf, "Output", out_volt, BMAX, 1, 0);
			    matfile_close(mf);
			    saved = 1;
			}

			// set next timer
			(*vda_out) = analog_value;
			(*rpm_measured) = curr_rpm;
			NiFpga_WriteU32(myrio_session, NiFpga_MyRio1900Fpga60_ControlU32_IRQTIMERWRITE,timeout_value);
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

    // parameter table definition
    char *prog_title = "PI Control Table";
    table prog_table[] = {
    		{"V_ref(rpm): ", 1, val_vref}, // V reference velocity in RPM , editable, initial value 0
    		{"V_J(rpm): ", 0, 0.0}, // inertia velocity, internally determined, init 0
    		{"V_out(mV): ",0,0.0}, // output velocity
    		{"K_p(Vs/r): ",1,val_kp}, // proportional constant
    		{"K_i(V/r): ",1,val_ki}, // integral constant
    		{"BTI(ms): ",1,0.001*timeout_value} // basic time increment
    };

    // stat verifier
    int32_t status_check;

    // thread resources struct declaration
    ThreadResource thread_resources;
    thread_resources.irqThreadReady = NiFpga_True;
    thread_resources.p_table = prog_table;

    // timer init
    MyRio_IrqTimer irq_timer;
    irq_timer.timerWrite = NiFpga_MyRio1900Fpga60_ControlU32_IRQTIMERWRITE;
    irq_timer.timerSet = NiFpga_MyRio1900Fpga60_ControlBool_IRQTIMERSETTIME;
    status_check = Irq_RegisterTimerIrq(&irq_timer,
           								&thread_resources.irqContext,
           								timeout_value);

    printf("Status Check: %d\n", status_check);
    pthread_t thread;
    status_check = pthread_create(&thread,
       							NULL,
       							ISR_TIMER,
       							&thread_resources);
    printf("Status Check: %d\n", status_check);




    // c2 table begins its own while loop
    ctable2(prog_title,prog_table,6);


    // end thread after table closes
    printf_lcd("\fEnded Program.");
    thread_resources.irqThreadReady = NiFpga_False;
    status_check = pthread_join(thread, NULL);
    printf("Status Check: %d\n", status_check);
    status_check = Irq_UnregisterTimerIrq(&irq_timer,
        						thread_resources.irqContext);
    printf("Status Check: %d\n", status_check);

	status = MyRio_Close();						// close FPGA session

	return status;
}
