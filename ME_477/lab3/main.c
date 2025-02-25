/*
 * Lab 3
 * Author: Dylan Rohauer
 * Date: 2/7/25
 * Description: This lab creates two functions;
 * one for putting a character on the myRio LCD
 * and another for reading the key pressed on an
 * array of buttons.
 */

/* includes */
#include <stdio.h>
#include <stdint.h>
#include "MyRio.h"
#include "T1.h"
#include "DIO.h"
#include "MyRio1900.h"
#include <UART.h>
#include <time.h>

/* prototypes */
int putchar_lcd(int c);
char getkey(void);
void wait(void);


int main(int argc, char **argv)
{
	NiFpga_Status status;

    status = MyRio_Open(); // open FPGA session
    if (MyRio_IsNotSuccess(status)) return status;

    // test putchar_lcd() with some characters
    int ret_val = putchar_lcd('\f');
    printf("Returned Value: %d\n",ret_val);
    ret_val = putchar_lcd('A');
    printf("Returned Value: %d\n",ret_val);
    ret_val = putchar_lcd('\n');
    printf("Returned Value: %d\n",ret_val);
    ret_val = putchar_lcd('C');
    printf("Returned Value: %d\n",ret_val);
    ret_val = putchar_lcd('C');
    printf("Returned Value: %d\n",ret_val);
    ret_val = putchar_lcd('\b');
    printf("Returned Value: %d\n",ret_val);
    ret_val = putchar_lcd('B');
    printf("Returned Value: %d\n",ret_val);
    ret_val = putchar_lcd(366);
    printf("Returned Value: %d\n",ret_val);
    ret_val = putchar_lcd(-25);
    printf("Returned Value: %d\n",ret_val);
    ret_val = putchar_lcd('\v');
    printf("Returned Value: %d\n",ret_val);
    ret_val = putchar_lcd(120);
    printf("Returned Value: %d\n",ret_val);

    // test getkey()
    char test = getkey();
    printf("\f\vRecieved Char: %c\n", test);
    test =  getkey();
    printf("\f\vRecieved Char: %c\n", test);

	// test functions which are dependent on
	// getkey() and putchar_lcd()
	printf_lcd("\fEnter Value: ");
   	char buf[40]={0};
   	char *myString = fgets_keypad(buf, 40);
   	printf("My string is: %s\n",myString);


	status = MyRio_Close();	// close FPGA session

	return status;
}

// function for printing LCD character
int putchar_lcd(int c){
	// Declare relevant variables and define
	// special characters to memory location
	int ret = 0;
	static int init = 0;
	static uint8_t com[5] = {12,17,8,128,13};
	static uint8_t *data;
	static uint8_t convert;
	static MyRio_Uart uart;
	static int32_t status;

	// Establish connection through UART to LCD
 	if (init==0){
 		uart.name = "ASRL2::INSTR";
 		uart.defaultRM = 0;
 		uart.session = 0;
 		status = Uart_Open(&uart,19200,8,Uart_StopBits1_0,Uart_ParityNone);
		if (status<VI_SUCCESS){
			printf("Connection Unsuccessful.\n");
			ret = EOF;
		}
		else{
			init = 1;
		}
	}
 	// Handle cases of special and out of
 	// range characters
 	if ((char)c=='\f'){
 		data = &(com[0]); // clear screen
 		status = Uart_Write(&uart,data,1);
 		data = &(com[1]); // backlight
 		status = Uart_Write(&uart,data,1);
 	}
 	else if((char)c=='\b'){
 		data = &(com[2]); // next space
 		status = Uart_Write(&uart,data,1);
 	}
 	else if((char)c=='\v'){
 	 	data = &(com[3]); // back to start
 	 	status = Uart_Write(&uart,data,1);
 	}
 	else if((char)c=='\n'){
 		data = &(com[4]); // newline
 		status = Uart_Write(&uart,data,1);
 	}
 	else if(c<0||c>255){
 		status = -1;
 		printf("Error: out of range.\n");
 	}
 	else{
 		convert = (uint8_t)c;
 		data = &convert;
 		status = Uart_Write(&uart,data,1);
 	}

 	// check return status
	if (status<VI_SUCCESS){
		ret = EOF;
	}
	else{
		ret = c;
	}

	return ret;
}

// function for getting keypad press value
char getkey(void){
	// initialize 8 digital channels
	static MyRio_Dio ch[8];
	int i; // channel index
	for(i=0;i<8;i++){
		ch[i].dir = NiFpga_MyRio1900Fpga60_ControlU8_DIOB_70DIR;
		ch[i].out = NiFpga_MyRio1900Fpga60_ControlU8_DIOB_70OUT;
		ch[i].in = NiFpga_MyRio1900Fpga60_IndicatorU8_DIOB_70IN;
		ch[i].bit = i;
	}
	const char table[4][4]={
			{'1','2','3',UP},
			{'4','5','6',DN},
			{'7','8','9',ENT},
			{'0','.','-',DEL}
	};

	// define while loop boolean, row,
	// and column indices
	NiFpga_Bool b = NiFpga_True;
	int col; // column index
	int row; // row index
	int o_col;
	while (b==NiFpga_True){
		for (col=0;col<4;col++){
			// enable high impedance for columns
			for(o_col=0;o_col<4;o_col++){
				Dio_ReadBit(&ch[o_col]);
			}

			// ground selected column
			Dio_WriteBit(&ch[col],NiFpga_False);

			// read each row & look for low reading
			for (row=4; row <8;row++){
				b = Dio_ReadBit(&ch[row]);
				if (b==NiFpga_False){
					break;
				}
			}
			if (b==NiFpga_False){
				break;
			}
			else{
				wait();
			}
		}
	}
	// wait for key to be released
	while ((b = Dio_ReadBit(&ch[row]))==NiFpga_False){
	}
	char k = table[row-4][col];
	return k;
}

// wait for a short period of time
void wait(void){
	uint32_t i;
	i = 417000;
	while(i>0){
		i--;
	}
	return;
}
