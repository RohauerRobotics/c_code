#include <stdio.h>

int main(int argc, char *argv[])
{
	int i = 0;
	printf("argc  is : %d\n",argc);
	if (argc == 1){
		printf("You only have 1 argument, try again!\n");
	}
	else if (argc >1 && argc <4){
		printf("Here are your arguments:\n");
		
		for (i=0; i<argc; i++){
				printf("%s ",argv[i]);
		}
		printf("\n");
	}
	else {
		printf("Too many arguments.\n");
	}
}
