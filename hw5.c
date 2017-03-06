#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]){
	printf("Hello World\n");
	int opt = getopt(argc, argv, "hdmt");
	
	while(opt != -1){
		
		opt = getopt(argc, argv, "hdmt");
	}
}
