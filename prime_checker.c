// Prime Checker Algorithm

#include <stdio.h>
#include <math.h>

int primality(int x);

int main(void){
	int i, j = 0;
	const int L = 10;
	int a[] = {0,1,4,11,121,4338,1901,7919,2022,2027}; // list of numbers to test
	int ap[L]; // list of affirmed prime numbers
	for (i=0; i < L; i++){
		if (primality(a[i])){
			ap[j] = a [i];
			j++;
		}
	}
	for (i=0; i<j;i++){
		printf("prime %d of %d is %d\n", i+1,j,ap[i]);
	}
}

int primality(int x){
	int i;
	int max_divisor =  floor(sqrt(x));
	if (x < 2) {return 0;} // 1 isn't a prime number 
	if (x%2==0 && x>2){return 0;} 
	for (i=3; i<max_divisor+1; i+=2){ // checks list of possible factors
		if (x%i == 0){return 0;}
	}
	return 1; // if no returns have been activated then this is a prime number
}
