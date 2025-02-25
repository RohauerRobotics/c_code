/*
 * Lab 0
 * Author: Dylan Rohauer
 * Date: 1/17/25
 * Description: Use Eclipse IDE and debugger with myRio
 */

/* includes */
#include <stdio.h>
#include "MyRio.h"
#include "T1.h"

/* prototypes */
int sumsq(int x); // sum of squares

/* definitions */
#define N 4 // number of loops

int main(int argc, char **argv)
{
	NiFpga_Status status;
	static int x[10]; // list of ten numbers
	static int  i; // index

    status = MyRio_Open();		    			// open FPGA session
    if (MyRio_IsNotSuccess(status)) return status;

    printf("Hello World!\n");				// Print to console

    printf_lcd("\fHello Dylan\n\n"); //Print to LCD display
    for (i=0;i<N;i++){
    	x[i] = sumsq(i);
    	printf_lcd("%d",x[i]);
    }

	status = MyRio_Close();						// close FPGA session

	return status;
}
static int y = 4;

int sumsq(int x){
	return y + x*x;
}

/*
When I first looked at the code in this I thought that the values printed from the length ten array of x
would be as follows: [4,5,8,13], since I assumed that static y would remain 4, rather than be assigned the
previous value. But after running the program and debugger I saw that the value of y was reassigned to the value
y = y + x^2 meaning that the output values were [4,5,9,18]. Where the value of i (and the input to sumsq)
was [0,1,2,3] (stopped before 4)and the value of y was [4,4,5,9].

I changed the function to directly return y + x^2 rather than define it as y and this gave me the result I
initially expected and now I see that this wasn't what the program was designed to do.
*/

