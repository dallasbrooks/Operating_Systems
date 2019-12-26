#include "diskhelp.h"

//print the contents of the disk image
char* Print_Disk(char* ad){
	char* init = ad;
	char* subdir = NULL;
	char name[21];
	char ext[4];
	do{
		while(ad[0] != 0x00){
			if(ad[26] != 0x00 && ad[26] != 0x01 && ad[11] != 0x0f && ((ad[11] & 0x08) != 0x08)){
				char type;
				int file_size = 0;
				if((ad[11] & 0x10) == 0x10){
					type = 'D';
					if(ad[0] == '.' || ad[1] == '.'){
						ad += 0x20;
						continue;
					}
				}
				else{
					type = 'F';
					file_size = (ad[28] & 0xff) + ((ad[29] & 0xff) << 8) + ((ad[30] & 0xff) << 16) + ((ad[31] & 0xff) << 24);
				}
				int a;
				for(a = 0; a < 8 && ad[a] != ' '; a++){
					name[a] = ad[a];
				}
				name[a] = '\0';
				for(a = 0; a < 3; a++){
					ext[a] = ad[a+8];
				}
				ext[a] = '\0';
				if(type == 'F'){
					strncat(name, ".", 1);
				}
				strncat(name, ext, 3);
				int year = ((ad[17] & 0xfe) >> 1) + 1980;
				int month = ((ad[16] & 0xe0) >> 5) + ((ad[17] & 0x01) << 3);
				int day = ad[16] & 0x1f;
				int hour = (ad[15] & 0xf8) >> 3;
				int min = ((ad[14] & 0xe0) >> 5) + ((ad[15] & 0x07) << 3);
				if((ad[11] & 0x02) == 0 && (ad[11] & 0x08) == 0){
					printf("%c %10u %20s %04d-%02d-%02d %02d:%02d\n", type, file_size, name, year, month, day, hour, min);
				}
				if((ad[11] & 0x10) == 0x10){
					subdir = ad;
					ad = Print_Disk(ad + 0x20);
					printf("\n");
					for(a = 0; a < 8; a++){
						printf("%c", subdir[a]);
					}
					printf("\n==================\n");
					ad = Print_Disk(init + (subdir[26] + 31 - 19) * SECTOR_SIZE);
				}
			} 
			ad += 0x20;
		}
		subdir = ad;
	}while(subdir != NULL && subdir[0] != 0x00);
	return subdir;
}

//main function to control program
int main(int argc, char* argv[]){
	if(argc != 2){
		printf("To use: ./disklist <disk>.ima\n");
		exit(1);
	}
	int disk;
	struct stat sb;
	char* ad;
	if((disk = open(argv[1], O_RDWR)) < 0){
		printf("Error: failed to read disk image.\n");
		exit(1);
	}
	fstat(disk, &sb);
	ad = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, disk, 0);
	if(ad == MAP_FAILED){
		printf("Error: failed to map memory.\n");
		exit(1);
	}
	printf("ROOT\n==================\n");
	Print_Disk(ad + SECTOR_SIZE * 19);
	munmap(ad, sb.st_size);
	close(disk);
	return 0;
}