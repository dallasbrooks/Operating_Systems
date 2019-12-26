#include "diskhelp.h"

//get the FAT entry at index K
int get_FAT(char* ad, int k){
	int low;
	int high;
	if((k % 2) == 0){
		low = ad[SECTOR_SIZE + (3*k/2) + 1] & 0x0f;
		high = ad[SECTOR_SIZE + (3*k/2)] & 0xff;
		return (low << 8) + high;
	}
	else{
		low = ad[SECTOR_SIZE + (3*k/2)] & 0xf0;
		high = ad[SECTOR_SIZE + (3*k/2) + 1] & 0xff;
		return (low >> 4) + (high << 4);
	}
}

//get the total size of the disk image
int Disk_Size(char* ad){
	int bytes = ad[11] + (ad[12] << 8);
	int sectors = ad[19] + (ad[20] << 8);
	return bytes * sectors;
}

//get the free space of the disk image
int Free_Disk(char* ad){
	int free = 0;
	int a;
	for(a = 19; a < ad[19] + (ad[20] << 8); a++){
		if(get_FAT(ad, a) == 0x000){
			free++;
		}
	}
	return free * SECTOR_SIZE;
}