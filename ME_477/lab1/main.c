/*
 * Lab 1
 * Author: Dylan Rohauer
 * Date: 01/26/25
 * Description: This lab builds a function to accept a double float
 * from the keypad and a function that prints values on the LCD itself.
 */

/* includes */
#include <stdio.h>
#include "MyRio.h"
#include "T1.h"
#include <string.h>
#include <stdarg.h>
/* prototypes */
double double_in(char *prompt);
int printf_lcd(const char *format,...);
/* definitions */

int main(int argc, char **argv)
{
	NiFpga_Status status;

    status = MyRio_Open();		    			// open FPGA session
    if (MyRio_IsNotSuccess(status)) return status;

    // Test of my printf_lcd() function
    int test = printf_lcd("\nHello There!");
    printf("Test 1 return: %d\n", test);

    // check double_in() function twice
    double val = double_in("Enter :");
    printf("First Value Entered: %lf\n",val);
    double val2 = double_in("Enter :");
    printf("Second Value Entered: %lf\n",val2);

	status = MyRio_Close();						// close FPGA session

	return status;
}

int printf_lcd(const char *format,...){
	int string_len;

	char buffer[80];

	va_list args; // get pointer to arguments

	// use string and pointer to arguments to build full string
	va_start(args,format);
		string_len = vsnprintf(buffer, 80, format, args);
	va_end(args);

	// print each string character to lcd
	int i = 0;
	for(i=0;i<string_len;i++){
		putchar_lcd(*(buffer+i));
	}

	return string_len;
}



double double_in(char *prompt){
	// initialize error
	int err = 1;
	char *out; // output string
	char *error_message = "";

	if (strlen(prompt) < 21){ //check input string length so it fits on one line
		while (err==1){
			// flush display
			printf_lcd("\f");
			printf_lcd("\n%s",error_message);
			printf_lcd("\v%s",prompt);

			// get user input
			char buff[40];
			char *user_in = fgets_keypad(buff, 40);

			// check for NULL keypad input
			if (user_in=='\0'){
				err = 1;
				error_message = "Nothing Entered!";
			}
			else{
				// check for arrows
				char *arrow;
				arrow = strpbrk(user_in,"[]");

				// define input length
				int user_len = strlen(user_in);

				// loop through input to check for extra negatives, past first entry
				int i = 0;
				int minus_count = 0;
				for (i=1;i<user_len;i++){
					if (user_in[i]=='-'){
						minus_count++;
					}
				}

				// extra decimal check, check for more than one decimal point
				int period_count = 0;
				for (i=0;i<user_len;i++){
					if (user_in[i]=='.'){
						period_count++;
					}
				}

				// arrow input check
				if (arrow != 0){
					err = 1;
					error_message = "Arrow Entered!";
				}
				// extra minus check
				else if (minus_count>0){
					err = 1;
					error_message = "Too Many Negatives!";
				}
				// extra period check
				else if (period_count>1){
					err = 1;
					error_message = "Too Many Periods!";
				}
				// no error, breaks while loop
				else{
					err = 0;
					out = user_in;
				}
			}
		}
		// exit while loop
		printf_lcd("\f");
		printf_lcd("\v%s%s",prompt,out);
	}
	// return double from string conversion
	double ret;
	sscanf(out,"%lf",&ret);
	return ret;
}
