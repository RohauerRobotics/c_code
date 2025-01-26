// strings and arrays example
#include <stdio.h>

int main(int argc, char *argv[]){
	int nums[4] = {0};
	char name[4] = {'E'};
	
	// print out name and numbers raw
	printf("numbers: %d %d %d %d\n",nums[0],nums[1],nums[2],nums[3]);
	// print name as a list of characters using %c
	printf("name(by letter): %c %c %c %c\n",name[0],name[1],name[2],name[3]);
	// print name as a string
	printf("name: %s\n",name);
	
	// assign values to list of numbers
	nums[0] = 1;
	nums[1] = 2;
	nums[2] = 3;
	nums[3] = 4;
	
	// assign letters to name
	name[0] = 'd';
	name[1] = 'y';
	name[2] = 'l';
	name[3] = '\0'; // NULL value indicating end of a string
	
	printf("numbers: %d %d %d %d\n",nums[0],nums[1],nums[2],nums[3]);
	
	printf("name(by letter): %c %c %c %c\n",name[0],name[1],name[2],name[3]);
	
	printf("name: %s\n",name);
	
	//another way to make a string
	char *another = "Dyl"; // uses a pointer to build string
	printf("another: %s\n",another);
	printf("another(as a list): %c %c %c %c \n", another[0],another[1],another[2],another[3]); 
	
	return 0;
}
