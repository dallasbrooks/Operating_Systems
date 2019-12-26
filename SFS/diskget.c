#include "diskhelp.h"

//copy the file from the disk image to the local directory
void Get_File(char* ad, char* ad2, char* fp, int file_size){
	int logical = fp[26] + (fp[27] << 8);
	int k;
	int bytes = file_size;
	int physical;
	do{
		if(bytes == file_size){
			k = logical;
		}
		else{
			k = get_FAT(ad, k);
		}
		physical = (31 + k) * SECTOR_SIZE;
		int a;
		for(a = 0; a < SECTOR_SIZE && bytes > 0; a++){
			ad2[file_size - bytes] = ad[a + physical];
			bytes--;
		}
	}while(bytes > 0);
}

//find the target file in the disk image
char* Find_File(char* ad, char* file){
	char* init = ad;
	while(ad[0] != 0x00){
		if(ad[0] != '.' && ad[1] != '.' && ad[26] != 0x00 && ad[26] != 0x01 && ad[11] != 0x0f && (ad[11] & 0x08) != 0x08){
			if((ad[11] & 0x10) != 0x10){
				char name[21];
				char ext[4];
				int a;
				for(a = 0; a < 8 && ad[a] != ' '; a++){
					name[a] = ad[a];
				}
				name[a] = '\0';
				for(a = 0; a < 3 && ad[a+8] != ' '; a++){
					ext[a] = ad[a+8];
				}
				ext[a] = '\0';
				strncat(name, ".", 1);
				strncat(name, ext, strlen(ext));
				if(strncmp(name, file, 25) == 0){
					return ad;
				}
			}
			else{
				char* found = Find_File(init + (ad[26] + 31 - 19) * SECTOR_SIZE, file);
				if(found != NULL){
					return found;
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
		printf("To use: ./diskget <disk>.ima <file>\n");
		exit(1);
	}
	int disk;
	struct stat sb;
	char* ad;
	if((disk = open(argv[1], O_RDWR)) < 0){
		printf("Error: failed to open disk.\n");
		exit(1);
	}
	fstat(disk, &sb);
	ad = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, disk, 0);
	if(ad == MAP_FAILED){
		close(disk);
		printf("Error: failed to map memory.\n");
		exit(1);
	}
	char* file_found = Find_File(ad + SECTOR_SIZE * 19, argv[2]);
	if(file_found == NULL){
		munmap(ad, sb.st_size);
		printf("File not found.\n");
		exit(1);
	}
	int file_size = (file_found[28] & 0xff) + ((file_found[29] & 0xff) << 8) + ((file_found[30] & 0xff) << 16) + ((file_found[31] & 0xff) << 24);
	FILE* fp = fopen(argv[2], "wb+");
	if(fp == NULL){
		munmap(ad, sb.st_size);
		close(disk);
		fclose(fp);
		printf("Error: failed to open file.\n");
		exit(1);
	}
	int disk2 = fileno(fp);
	if(fseek(fp, file_size-1, SEEK_SET) == -1){
		munmap(ad, sb.st_size);
		close(disk);
		fclose(fp);
		close(disk2);
		printf("Error: failed to seek end of file.\n");
		exit(1);
	}
	if(write(disk2, "", 1) != 1){
		munmap(ad,sb.st_size);
		close(disk);
		fclose(fp);
		close(disk2);
		printf("Error: failed to write NULL character at end of file.\n");
		exit(1);
	}
	char* ad2 = mmap(NULL, file_size, PROT_WRITE, MAP_SHARED, disk2, 0);
	if(ad2 == MAP_FAILED){
		printf("Error: failed to map memory.\n");
		exit(1);
	}
	Get_File(ad, ad2, file_found, file_size);
	munmap(ad, sb.st_size);
	munmap(ad2, file_size);
	close(disk);
	close(disk2);
	fclose(fp);
	return 0;
}