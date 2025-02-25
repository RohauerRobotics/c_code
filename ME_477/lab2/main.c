/*
 * Lab 2
 * Author: Dylan Rohauer
 * Date: 2/2/25
 * Description: The goal of this lab is to create a getchar_keypad()
 * function which retrieves a character from the keypad and allows
 * the user to add and delete numbers until the ENTR key is pressed.
 */

/* includes */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "MyRio.h"
#include "T1.h"

/* prototypes */
char* fgets_keypad(char *buf, int buflen);
int getchar_keypad(void);


int main(int argc, char **argv)
{
	NiFpga_Status status;

    status = MyRio_Open();		    			// open FPGA session
    if (MyRio_IsNotSuccess(status)) return status;

    printf_lcd("\fEnter Value: ");		// Print to LCD display

    char buf[40]={0};
    char *myString = fgets_keypad(buf, 40);
    printf("My string is: %s\n",myString);

    printf_lcd("\fEnter Value: ");
    char buf2[40]={0};
    char *myString2 = fgets_keypad(buf2, 40);
    printf("My string2 is: %s\n",myString2);

    printf_lcd("\fEnter Value: ");
    char buf3[40]={0};
    char *myString3 = fgets_keypad(buf3, 40);
    printf("My string3 is: %s\n",myString3);

	status = MyRio_Close();						// close FPGA session

	return status;
}

char* fgets_keypad(char *buf, int buflen){
	char *bufend = (buf+buflen-1); // end of buffer
	char *b_p = buf; // buffer pointer
	int c; // c is the value received from getchar_keypad()
	while ((c=getchar_keypad())!= EOF){
		if (b_p < bufend){
			*b_p=c;
			b_p = b_p +1;

		}
	if (b_p == buf){
		return NULL;
	}
	*bufend = '\0';
	}
	return buf;
}



int getchar_keypad(void){
	static char char_buf[40] = {0};
	static char *char_b_p = char_buf;
	static int n = 0;
	static int c = 0;
	static int ret = 0;
	if(n==0){
		char_b_p = &char_buf[0];
		while ((c=getkey())!= ENT){
			if(n<40){
				if (c==DEL&&n==0){
					printf("Delete Key Pressed and Nothing to Delete!\n");

				}
				else if (c==DEL&&n>0){
					putchar_lcd('\b');
					putchar_lcd(' ');
					putchar_lcd('\b');
					n = n - 1;
					printf("Buffer before DEL: %s\n",char_buf);
					*(char_b_p+n) = 0;
					printf("Buffer after DEL: %s\n",char_buf);
//					n = n - 1;
				}
				else{
					*(char_b_p+n) = c;
					n = n + 1;
					putchar_lcd(c);
				}
			}
		}
		printf("Buffer: %s\n", char_buf);
		n = n + 1;
		char_b_p = &char_buf[0];
	}
	if (n>1){
		n=n-1;
		ret = *(char_b_p);
		*(char_b_p) = 0;
		char_b_p = char_b_p +1;
	}
	else{
		n=n-1;
		*(char_b_p) = 0;
		ret = EOF;
	}
	return ret;
}
