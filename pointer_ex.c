#include <stdio.h>

// always include main() or the code won't run
int main(void){
	int x = 1;
	int y = 5; 
	printf("x= %d\n", x); // print values
	printf("y= %d\n", y);
	int *pointer = &x; // pointer points to address of x
	int z = *pointer; // z reads the value stored in the address of the pointer
	printf("pointer points to an address with the value of %d which is x\n",*pointer);
	*pointer = 15; // assign pointer value to 15
	printf("\npointer has been assigned value of 15\n");
	printf("x now has a value of %d\n", x); // x now has the reassigned value of the pointer
}

