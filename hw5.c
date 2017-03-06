#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#define STR "hello\n"

int main(int argc, char* argv[]){
	bool enable_h = false, enable_m = false, enable_t = false;
	int count = 1;
	char* d_arg = "_rev0";
	int rev = 0;
	int opt = getopt(argc, argv, "hd:mt");
	while(opt != -1){
		switch(opt){
			case 'h':
			enable_h = true;
			break;
			case 'd':
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
	char* file = argv[optind];
	int fd = inotify_init();
	int wd = inotify_add_watch(fd, file, IN_MODIFY);
	if(wd == -1){
		perror("inotify_add_watch");
		return EXIT_FAILURE;
	}
	int y = open(file, O_RDWR);
	const size_t data_size = 100;
	char data[data_size];
	const size_t size = 5;
	char d[size];
	char* p;
	int x, num_bytes_read = 1;
	int backup = open(d_arg,  O_RDWR | O_CREAT | O_TRUNC, 
		S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
	while(num_bytes_read != 0){
		num_bytes_read = read(y, d, size);
		write(backup, d, num_bytes_read);
	}
	d_arg = "_rev" + count;
	printf("%s\n", d_arg);
	close(y);
	close(backup);
	while(1){
		x = read(fd, data, data_size);
		for(p = data; p < data + x;){
			struct inotify_event* event = (struct inotify_event*)p;
			if((event->mask & IN_MODIFY) != 0){
				backup = open(d_arg, O_RDWR | O_CREAT | O_TRUNC, 
					S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
				num_bytes_read = 1;
				printf("The file has been modified.\n");
				y = open(file, O_RDWR);
				while(num_bytes_read != 0){
					num_bytes_read = read(y, d, size);
					write(backup, d, num_bytes_read);
				}
				close(y);
				close(backup);
				count++;
				d_arg = "_rev" + count;
				printf("%s\n", d_arg);
			}
			p += sizeof(struct inotify_event) + event->len;
		}
	}
	close(fd);
}
