// while loop example
#include <stdio.h>

int main(int argc,char *argv[]){
	int i = 0; //counter variable
	while (i<25){
		printf("%d ",i);
		i++;
	}
	printf("\n");
	
	// this loop prints user arguments to file
	for(i=1;i<argc;i++){
		printf("arg %d is %s\n",i,argv[i]);
	}
	char *states[] = {
		"California","Oregon",
		"Washington", "Texas"
	};
	
	int num_states = 4;
	
	for (i=0;i<num_states;i++){
		printf("state %d is %s\n",i+1,states[i]);
	}
	
	return 0;
}
