// switches and vowels
#include <stdio.h>

int main(int argc, char *argv[]){
	if (argc != 2){
		printf("Error you need 1 argument!\n");
		return 1; // aborts the program
	}
	
	int i = 0; //counter initialization
	
	for (i=0;argv[1][i]!='\0';i=i+1){
		char letter = argv[1][i];
		
		switch (letter){
			case 'a':
			case 'A':
				printf("%d: 'A'\n",i);
				break;
			case 'e':
			case 'E':
				printf("%d: 'E'\n",i);
				break;
			case 'i':
			case 'I':
				printf("%d: 'I'\n",i);
				break;
			case 'o':
			case 'O':
				printf("%d: 'O'\n",i);
				break;
			case 'u':
			case 'U':
				printf("%d: 'U'\n",i);
				break;
			case 'y':
			case 'Y':
				printf("%d: 'Y'\n",i);
				break;
			default:
				printf("%d: %c is not a vowel\n",i,letter);
		}
	}
	return 0;
}
