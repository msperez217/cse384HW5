#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <libgen.h>
#include <time.h>
#include <utime.h>

int main(int argc, char* argv[]){
	bool enable_h = false, enable_m = false, enable_t = false, enable_d_arg = false, enable_error = false;
	char* d_arg = NULL;
	int rev = 0;
	int opt = getopt(argc, argv, ":hd:mt");
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
			case ':':
			enable_d_arg = true;
			break;
			default:
			enable_error = true;
			break;
		}
		opt = getopt(argc, argv, ":hd:mt");
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

	if(enable_error == true){
		printf("ERROR: Invalid option found. Valid options are h, d, m, and t.\n");
		return EXIT_FAILURE;
	}

	if(enable_d_arg == true){
		printf("ERROR: Option -d requires an argument.\n");
		return EXIT_FAILURE;

	}
	if(argv[optind] == NULL){
		printf("No argument given.\n");
		printf("Program Name: %s\n", argv[0]);
		return EXIT_SUCCESS;
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

	char* file_name = basename(argv[optind]);
	char file_loc[100];
	if(d_arg == NULL){
		char file_origin[100];
		snprintf(file_origin, 100,"%s",argv[optind]);
		snprintf(file_loc, 100,"%s/%s",dirname(file_origin),file_name);
	}
	else{
		snprintf(file_loc, 100,"%s/%s",d_arg,file_name);
	}

	const size_t data_size = 100;
	char data[data_size];
	const size_t size = 5;
	char d[size];
	char* p;
	char backupFile[100];

	if(enable_t == true){
		time_t tim = time(NULL);
		if((int)time == -1){
			perror("time");
			return EXIT_FAILURE;
		}
		struct tm appendTime = *localtime(&tim);
		snprintf(backupFile, 100,"%s_%d%d%d%d%d%d", file_loc, appendTime.tm_year + 1900, appendTime.tm_mon + 1, appendTime.tm_mday,
			appendTime.tm_hour, appendTime.tm_min, appendTime.tm_sec);
	}
	else{
		snprintf(backupFile, 100, "%s_rev%d",file_loc, rev);
	}

	int x, num_bytes_read = 1;

	int backup = open(backupFile,  O_RDWR | O_CREAT | O_TRUNC, 
		S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
	int y = open(file, O_RDWR);

	while(num_bytes_read != 0){
		num_bytes_read = read(y, d, size);
		write(backup, d, num_bytes_read);
	}
	close(y);
	close(backup);

	if(enable_m == false){
		struct stat file_stat;
		struct utimbuf time_f;
		if(stat(file, &file_stat) == -1){
			perror("stat");
			return EXIT_FAILURE;
		}
		if(chown(backupFile,file_stat.st_uid,file_stat.st_gid) == -1){
			perror("chown");
			return EXIT_FAILURE;
		}
		if(chmod(backupFile,file_stat.st_mode) == -1){
			perror("chmod");
			return EXIT_FAILURE;
		}
		time_f.actime = file_stat.st_atim.tv_sec;
		time_f.modtime = file_stat.st_mtim.tv_sec;
		if(utime(backupFile, &time_f) == -1){
			perror("utime");
			return EXIT_FAILURE;
		}
	}

	printf("The backup file was sent to %s\n", backupFile);
	while(1){
		x = read(fd, data, data_size);
		if(access(file, F_OK) == -1){
			printf("The file has been deleted. No more backups will be made.\n");
			return EXIT_FAILURE;
		}
		for(p = data; p < data + x;){
			struct inotify_event* event = (struct inotify_event*)p;
			if((event->mask & IN_MODIFY) != 0){
				if(enable_t == true){
					time_t tim = time(NULL);
					if((int)time == -1){
						perror("time");
						return EXIT_FAILURE;
					}
					struct tm appendTime = *localtime(&tim);
					snprintf(backupFile, 100,"%s_%d%d%d%d%d%d", file_loc, appendTime.tm_year + 1900, appendTime.tm_mon + 1, appendTime.tm_mday,
						appendTime.tm_hour, appendTime.tm_min, appendTime.tm_sec);
				}
				else{
					rev++;
					snprintf(backupFile, 100, "%s_rev%d",file_loc, rev);
				}
				backup = open(backupFile, O_RDWR | O_CREAT | O_TRUNC, 
					S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
				num_bytes_read = 1;
				y = open(file, O_RDWR);
				while(num_bytes_read != 0){
					num_bytes_read = read(y, d, size);
					write(backup, d, num_bytes_read);
				}
				close(y);
				close(backup);
				if(enable_m == false){
					struct stat file_stat;
					struct utimbuf time_f;
					if(stat(file, &file_stat) == -1){
						perror("stat");
						return EXIT_FAILURE;
					}
					if(chown(backupFile,file_stat.st_uid,file_stat.st_gid) == -1){
						perror("chown");
						return EXIT_FAILURE;
					}
					if(chmod(backupFile,file_stat.st_mode) == -1){
						perror("chmod");
						return EXIT_FAILURE;
					}
					time_f.actime = file_stat.st_atim.tv_sec;
					time_f.modtime = file_stat.st_mtim.tv_sec;
					if(utime(backupFile, &time_f) == -1){
						perror("utime");
						return EXIT_FAILURE;
					}
				}
				printf("The backup file was sent to %s\n", backupFile);
			}
			p += sizeof(struct inotify_event) + event->len;
		}
	}
	free(d_arg);
}
