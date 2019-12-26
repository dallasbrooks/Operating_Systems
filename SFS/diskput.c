#include "diskhelp.h"

//update the diks image with the file's information
void Update_Disk(char* ad, char* file, int file_size, int logical){
	//find available space in sector
	while(ad[0] != 0x00){
		ad += 0x20;
	}
	//set the filename and extension
	int a;
	int b = -1;
	for(a = 0; a < 8; a++){
		if(file[a] == '.'){
			b = a;
			break;
		}
		if(b == -1){
			ad[a] = file[a];
		}
		else{
			ad[a] = ' ';
		}
	}
	for(a = 0; a < 3; a++){
		ad[a+8] = file[a+b+1];
	}
	//set the attribute
	ad[11] = 0x00;
	//set the creation date and time
	time_t t = time(NULL);
	struct tm* now = localtime(&t);
	int year = now->tm_year + 1900;
	int month = (now->tm_mon + 1);
	int day = now->tm_mday;
	int hour = now->tm_hour;
	int minute = now->tm_min;
	ad[14] = 0x00;
	ad[15] = 0x00;
	ad[16] = 0x00;
	ad[17] = 0x00;
	ad[17] |= (year - 1980) << 1;
	ad[17] |= (month - ((ad[16] & 0xe0) >> 5)) >> 3;
	ad[16] |= (month - (((ad[17] & 0x01)) << 3)) << 5;
	ad[16] |= (day & 0x1f);
	ad[15] |= (hour << 3) & 0xf8;
	ad[15] |= (minute - ((ad[14] & 0xe0) >> 5)) >> 3;
	ad[14] |= (minute - ((ad[15] & 0x07) << 3)) << 5;
	//set the first logical cluster
	ad[26] = (logical - (ad[27] << 8)) & 0xff;
	ad[27] = (logical - ad[26]) >> 8;
	//set the file size
	ad[28] = file_size & 0x000000ff;
	ad[29] = (file_size & 0x0000ff00) >> 8;
	ad[30] = (file_size & 0x00ff0000) >> 16;
	ad[31] = (file_size & 0xff000000) >> 24;
}

//get the next free space in the disk image
int Free_FAT(char* ad){
	int a = 2;
	char* curr = ad + (a + 30) * SECTOR_SIZE;
	while(get_FAT(ad, a) != 0x00 || curr[0] != 0x00){
		a++;
		curr = ad + (a + 31) * SECTOR_SIZE;
	}
	return a;
}

//set the FAT entry k to value
void Set_FAT(char* ad, int k, int value){
	if(k%2 == 0){
		ad[SECTOR_SIZE + (3*k/2) + 1] = (value >> 8) & 0x0f;
		ad[SECTOR_SIZE + (3*k/2)] = value & 0xff;
	}
	else{
		ad[SECTOR_SIZE + (3*k/2)] = (value << 4) & 0xf0;
		ad[SECTOR_SIZE + (3*k/2) + 1] = (value >> 4) & 0xff;
	}
}

//put the target file on the disk image
void Put_File(char* ad, char* ad2, char* fp, char* file, int file_size){
	int current = Free_FAT(ad);
	int bytes = file_size;
	int physical;
	if(ad != fp){
		Update_Disk(ad + (fp[26] + 31) * SECTOR_SIZE, file, file_size, current);
	}
	else{
		Update_Disk(ad + SECTOR_SIZE * 19, file, file_size, current);
	}
	do{
		physical = (current + 31) * SECTOR_SIZE;
		int a;
		for(a = 0; a < SECTOR_SIZE; a++){
			if(bytes <= 0){
				Set_FAT(ad, current, 0xfff);
				break;
			}
			ad[a + physical] = ad2[file_size - bytes];
			bytes--;
		}
		Set_FAT(ad, current, 0x77);
		int next = Free_FAT(ad);
		Set_FAT(ad, current, next);
		get_FAT(ad, current);
		current = next;
	}while(bytes > 0);
}

//find the specified directory to store the file
char* Find_Dir(char* ad, char* dir){
	char* init = ad;
	char* found_file = NULL;
	char* subdir = NULL;
	char name[21];
	while(ad[0] != 0x00){
		if(ad[26] != 0x00 && ad[26] != 0x01 && ad[11] != 0x0f && (ad[11] & 0x08) != 0x08){
			int a;
			for(a = 0; a < 8 && ad[a] != ' '; a++){
				name[a] = ad[a];
			}
			name[a] = '\0';
			if(!strncmp(name, dir, 25)){
				return ad;
			}
			if((ad[11] & 0x10) == 0x10){
				subdir = ad;
				ad += 0x20;
				found_file = Find_Dir(ad, dir);
				if(found_file != NULL){
					return found_file;
				}
				else{
					found_file = Find_Dir(init + (subdir[26] + 31 - 19) * SECTOR_SIZE, dir);
				}
			}
		}
		ad += 0x20;
	}
	return NULL;
}

//main function to control program
int main(int argc, char* argv[]){
	if(argc != 3){
		printf("to use: ./diskput <disk>.ima [directory path]<file>\n");
		exit(1);
	}
	int has_path = 0;
	int a;
	for(a = 0; a < strlen(argv[2]); a++){
		if(argv[2][a] == '/'){
			has_path = 1;
			break;
		}
	}
	char* file;
	char* subdir = NULL;
	char* tokens[256];
	int count = 1;
	if(has_path == 1){
		char* token = strtok(argv[2], "/");
		while(token != NULL){
			tokens[count++] = token;
			token = strtok(NULL, "/");
		}
		tokens[a] = NULL;
		subdir = strdup(tokens[count-2]);
		file = strdup(tokens[count-1]);
	}
	else{
		file = argv[2];
	}
	int disk;
	struct stat sb;
	char* ad;
	if((disk = open(argv[1], O_RDWR)) < 0){
		printf("Error: failed to open disk.\n");
		exit(1);
	}
	fstat(disk, &sb);
	ad = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, disk, 0);
	if(ad == MAP_FAILED){
		printf("Error: failed to map memory.\n");
		exit(1);
	}
	int disk2;
	struct stat sb2;
	char* ad2;
	if((disk2 = open(file, O_RDWR)) < 0){
		printf("File not found.\n");
		exit(1);
	}
	fstat(disk2, &sb2);
	ad2 = mmap(NULL, sb2.st_size, PROT_READ, MAP_SHARED, disk2, 0);
	if(ad2 == MAP_FAILED){
		printf("Error: failed to map memory.\n");
		exit(1);
	}
	if(sb2.st_size >= Free_Disk(ad)){
		printf("Not enough free space in the disk image.\n");
		exit(1);
	}
	char* subdir_ad;
	if(has_path == 1){
		subdir_ad = Find_Dir(ad + SECTOR_SIZE * 19, subdir);
		if(subdir_ad == NULL){
			printf("The directory not found.\n");
			exit(1);
		}
	}
	else{
		subdir_ad = ad;
	}
	Put_File(ad, ad2, subdir_ad, file, sb2.st_size);
	munmap(ad, sb.st_size);
	munmap(ad2, sb2.st_size);
	close(disk);
	close(disk2);
	return 0;
}