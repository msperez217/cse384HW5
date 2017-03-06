#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char* argv[]){
	bool enable_h = false;
	bool enable_d = false;
	bool enable_m = false;
	bool enable_t = false;
	char* d_arg = NULL;
	int opt = getopt(argc, argv, "hd:mt");
	while(opt != -1){
		switch(opt){
			case 'h':
			enable_h = true;
			break;
			case 'd':
			enable_d = true;
			d_arg = strdup(optarg);
			break;
			case 'm':
			enable_m = true;
			break;
			case 't':
			enable_t = true;
			break;
		}
		opt = getopt(argc, argv, "hd:mt");
	}
}
