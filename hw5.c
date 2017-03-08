#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

int main(int argc, char* argv[]){
	bool enable_h = false, enable_m = false, enable_t = false;
	char* d_arg = NULL;
	int rev = 0;
	char buffer[50];
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
	if(enable_h == true){
		printf("-------------------------------------------------------------\n");
		printf("This program maintains a backup of a file.\nIt takes a file name and its path as an argument.\nIt also accepts four options.\n"
			"These options are h, d, m, and y. The -h option gives you more info about the program and will terminate it once its done\n"
			"The -d option accepts an argument that allows the user to choose the backup location of the file.\nThe -m option disables "
			"meta-data duplication.\nThe -t option will append the duplication time to the file name.\n");
		printf("-------------------------------------------------------------\n");
		return EXIT_SUCCESS;
	}

	if(enable_m == true){

	}

	if(enable_t == true){
		
	}



	char* file = argv[optind];
	int fd = inotify_init();
	int wd = inotify_add_watch(fd, file,IN_DELETE | IN_MODIFY);

	if(fd == -1){
		perror("inotify_init");
		return EXIT_FAILURE;
	}

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
	sprintf(buffer, "%s_rev%d",file,rev);
	int x, num_bytes_read = 1;
	int backup = open(buffer,  O_RDWR | O_CREAT | O_TRUNC, 
		S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
	while(num_bytes_read != 0){
		num_bytes_read = read(y, d, size);
		write(backup, d, num_bytes_read);
	}
	close(y);
	close(backup);
	while(1){
		x = read(fd, data, data_size);
		for(p = data; p < data + x;){
			struct inotify_event* event = (struct inotify_event*)p;
			if((event->mask & IN_MODIFY) != 0){
				rev++;
				sprintf(buffer, "%s_rev%d",file, rev);
				backup = open(buffer, O_RDWR | O_CREAT | O_TRUNC, 
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
			}
			p += sizeof(struct inotify_event) + event->len;
		}
	}
	close(fd);
}
