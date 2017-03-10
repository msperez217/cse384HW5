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
		printf("This program maintains a backup of a file.\nEverytime the file is modified a new backup file will be made.\n"
			"It takes a file name and its path as an argument.\n"
			"It also accepts four options (h, d, m, and t).\n"
			"The -h option gives you more info about the program and will terminate it once its done\n"
			"The -d option accepts an argument that changes the location of the backup file.\n"
			"Otherwise the backup files will be stored where the original file is located.\n"
			"The -m option disables meta-data duplication.\nThe -t option will append the duplication time to the file name.\n");
		printf("-------------------------------------------------------------\n");
		free(d_arg);
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
		printf("Usage: %s FILENAME\n", argv[0]);
		return EXIT_SUCCESS;
	}

	char* file = argv[optind];
	int fd = inotify_init();
	int wd = inotify_add_watch(fd, file,IN_ATTRIB | IN_MODIFY);

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

	const size_t data_size = 1000;
	char data[data_size];
	const size_t size = 5;
	char d[size];
	char* p;
	char backupFile[100];

	if(enable_t == true){
		time_t tim = time(NULL);
		struct tm appendTime = *localtime(&tim);
		snprintf(backupFile, 100,"%s_%d%d%d%d%d%d", file_loc, appendTime.tm_year + 1900, appendTime.tm_mon + 1, appendTime.tm_mday,
			appendTime.tm_hour, appendTime.tm_min, appendTime.tm_sec);
	}

	else{
		snprintf(backupFile, 100, "%s_rev%d",file_loc, rev);
	}

	int backup = open(backupFile,  O_RDWR | O_CREAT | O_TRUNC, 
		S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
	if(backup == -1){
		perror("open");
		return EXIT_FAILURE;
	}
	int y = open(file, O_RDWR);
	if(y == -1){
		perror("open");
		return EXIT_FAILURE;
	}
	char* location = strdup(file_loc);
	printf("The file backups are located in %s\n", dirname(location));

	int num_bytes_read = 1;
	while(num_bytes_read != 0){
		num_bytes_read = read(y, d, size);
		if(num_bytes_read == -1){
			perror("read");
			return EXIT_FAILURE;
		}
		if(write(backup, d, num_bytes_read) == -1){
			perror("write");
			return EXIT_FAILURE;
		}
	}

	if(close(y) == -1){
		perror("close");
		return EXIT_FAILURE;
	}
	
	if(close(backup) == -1){
		perror("close");
		return EXIT_FAILURE;
	}

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

	int x;
	while(1){
		x = read(fd, data, data_size);
		if(x == -1){
			perror("read");
			return EXIT_FAILURE;
		}
		if(access(file, F_OK) == -1){
			printf("The file has been deleted. No more backups will be made.\n");
			return EXIT_FAILURE;
		}
		for(p = data; p < data + x;){
			struct inotify_event* event = (struct inotify_event*)p;
			if(enable_m == false){
				if((event->mask & IN_ATTRIB) != 0){
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
			}
			if((event->mask & IN_MODIFY) != 0){
				if(enable_t == true){
					time_t tim = time(NULL);
					struct tm appendTime = *localtime(&tim);
					snprintf(backupFile, 100,"%s_%d%d%d%d%d%d", file_loc, appendTime.tm_year + 1900, appendTime.tm_mon + 1, appendTime.tm_mday,
						appendTime.tm_hour, appendTime.tm_min, appendTime.tm_sec);
				}
				else{
					rev++;
					snprintf(backupFile, 100, "%s_rev%d",file_loc, rev);
				}
				backup = open(backupFile, O_RDWR | O_CREAT | O_TRUNC, 
					S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);

				if(backup == -1){
					perror("open");
					return EXIT_FAILURE;
				}

				y = open(file, O_RDWR);

				if(y == -1){
					perror("open");
					return EXIT_FAILURE;
				}

				num_bytes_read = 1;
				while(num_bytes_read != 0){
					num_bytes_read = read(y, d, size);
					if(num_bytes_read == -1){
						perror("read");
						return EXIT_FAILURE;
					}
					if(write(backup, d, num_bytes_read) == -1){
						perror("write");
						return EXIT_FAILURE;
					}
				}

				if(close(y) == -1){
					perror("close");
					return EXIT_FAILURE;
				}

				if(close(backup) == -1){
					perror("close");
					return EXIT_FAILURE;
				}

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
			}
			p += sizeof(struct inotify_event) + event->len;
		}
	}
	free(d_arg);
	free(location);
	return EXIT_SUCCESS;
}
