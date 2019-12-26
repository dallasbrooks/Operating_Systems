#include "diskhelp.h"

//get the OS name
void OS_Name(char* ad, char* os){
	int a;
	for(a = 0; a < 8; a++){
		os[a] = ad[a+3];
	}
}

//get the label of the disk
void Disk_Label(char* ad, char* label){
	ad += SECTOR_SIZE * 19;
	int a;
	while(ad[0] != 0x00){
		if(ad[11] == 0x08){
			for(a = 0; a < 8; a++){
				label[a] = ad[a];
			}
			break;
		}
		ad += 0x20;
	}
}

//count the number of files in the disk image
int Count_Files(char* ad){
	int count = 0;
	char* init = ad;
	while(ad[0] != 0x00){
		if(ad[0] != '.' && ad[1] != '.' && ad[26] != 0x00 && ad[26] != 0x01 && ad[11] != 0x0f && (ad[11] & 0x08) != 0x08){
			if((ad[11] & 0x10) != 0x10){
				count++;
			}
			else{
				count += Count_Files(init + (ad[26] + 31 - 19) * SECTOR_SIZE);
			}
		}
		ad += 0x20;
	}
	return count;
}

//print information about the disk image
void Print_Disk(char* os, char* label, int size, int free, int files, int fat, int sectors){
	printf("OS Name: %s\n", os);
	printf("Label of the disk: %s\n", label);
	printf("Total size of the disk: %d bytes\n", size);
	printf("Free size of the disk: %d bytes\n", free);
	printf("\n==============\n");
	printf("The number of files in the disk (including all files in the root directory and files in all subdirectories): %d\n", files);
	printf("\n=============\n");
	printf("Number of FAT copies: %d\n", fat);
	printf("Sectors per FAT: %d\n", sectors);
}

//main function to control program
int main(int argc, char* argv[]){
	if(argc != 2){
		printf("To use: ./diskinfo <disk>.ima\n");
		exit(1);
	}
	int disk;
	struct stat sb;
	char* ad;
	if((disk = open(argv[1], O_RDONLY)) < 0) {
		printf("Error: failed to read disk image\n");
		exit(1);
	}
	fstat(disk, &sb);
	ad = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, disk, 0);
	if(ad == MAP_FAILED){
		printf("Error: failed to map memory\n");
		exit(1);
	}
	char os[16];
	OS_Name(ad, os);
	char label[16];
	Disk_Label(ad, label);
	int size = Disk_Size(ad);
	int free = Free_Disk(ad);
	int files = Count_Files(ad + SECTOR_SIZE * 19);
	int fat = ad[16];
	int sectors = ad[22] + (ad[23] << 8);
	Print_Disk(os, label, size, free, files, fat, sectors);
	munmap(ad, sb.st_size);
	close(disk);
	return 0;
}