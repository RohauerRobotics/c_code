// size of arrays and strings
# include <stdio.h>

int main(int argc, char *argv[]){
	int areas[] = {10,12,12,14,20};
	char name[] = "Dylan";
	char full_name[] = {
	'D','y','l','a','n',' ','R','o','h','a','u','e','r','\0'
	};
	
	printf("The size of an int is %ld\n",sizeof(int));
	printf("The size of areas (int[]) is: %ld\n", sizeof(areas));
	printf("The first area is %d and the second is %d\n", areas[0],areas[1]);
	printf("The size of a char is %ld\n",sizeof(char));
	printf("The size of name is %ld\n",sizeof(name));
	printf("The number of chars is %ld\n",sizeof(name)/sizeof(char));
	printf("The size of full name is %ld\n", sizeof(full_name));
	printf("The number of characters in full name is %ld\n", sizeof(full_name)/sizeof(char));
	
	printf("name is %s and full name is %s\n",name,full_name);
	
	return 0;
}
