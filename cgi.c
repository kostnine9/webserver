#include <stdio.h>
#define N 3

int main(int argc, char **argv,char *env){
       if(argc < 2){
       		printf("Invalid arguments!");
		return -1;
	}
       int a, b;
       sscanf(argv[1], "%d", &a);
       sscanf(argv[2], "%d", &b);
	printf("The sum of %d and %d is %d\n", a, b, a+b);
	return 0;
}	
