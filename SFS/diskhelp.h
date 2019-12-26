#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define SECTOR_SIZE 512

int get_FAT(char* ad, int k);
int Disk_Size(char* ad);
int Free_Disk(char* ad);