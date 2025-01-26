// pointers, dreaded pointers
#include <stdio.h>

int main(int argc, char *argv[]){
	// create two arrays we care about
	int ages[] = {23,43,12,89,2};
	char *names[] = {
		"Alan", "Frank",
		"Mary", "John","Lisa"
	};
	// safely get the size of ages
	int count  = sizeof(ages)/sizeof(int); // number of people
	int i = 0;
	
	// first way using indexing
	for (i=0;i<count;i++){
		printf("%s has %d years alive.\n",names[i],ages[i]);
	}
	
	printf("----\n");
	
	// set up pointers to the start of arrays
	int *cur_age = &ages[0]; // this line is equivalent to int *cur_age = ages; 
	char **cur_name = names;
	// points to the first address of the list of numbers
	
	// test pointer value
	printf("cur_age addy: %p\n", &cur_age);
	printf("cur_age memory %d\n", *cur_age);
	
	// increment 1 value
	printf("cur_age addy 2: %p\n", (void *)(cur_age+1));
	printf("cur_age memory 2: %d\n", *(cur_age+1));
	
	for (i=0;i<count;i++){
		printf("%s is %d years old.\n",*(cur_name+i),*(cur_age+i));
	}
	
	printf("----\n");
	
	for (i=0;i<count;i++){
		printf("%s is %d years old again.\n", cur_name[i],cur_age[i]);
	}
	printf("----\n");
	for (cur_name = names, cur_age = ages;(cur_age-ages)<count;cur_name++,cur_age++){
		printf("%s lived %d years so far.\n", *cur_name, *cur_age);
	}
	return 0;
}
