// writing functions
#include <stdio.h>
#include <ctype.h> // holds isalpha() and isblank() functions

// forward declarations (prototypes)
// allows you to use a function before it is written
int can_print_it(char ch); // accepts a single character
void print_letters(char arg[]); // accepts a string

// calls print letters function for each argument string
void print_arguments(int argc, char *argv[]){
	int i = 0;
	for (i=0; i<argc;i++){
		print_letters(argv[i]); // function is used before it is written
					// this is okay because we used a forward declaration
	}
}
void print_letters(char arg[]){
	int i = 0;
	for (i=0;arg[i]!='\0';i++){
		char ch = arg[i];
		if (can_print_it(ch)){
			printf("'%c' == %d\n", ch, ch); // prints each letter as an integer value
		}
	}
	printf("---\n");
}

int can_print_it(char ch){
	return isalpha(ch) || isblank(ch);
}

int main(int argc, char *argv[]){
	print_arguments(argc,argv);
	return 0;
}
