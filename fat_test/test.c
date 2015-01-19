#include "test.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "fat.h"

FILE *fp;

int write_sector(int sector, uint8_t *data) {
	fseek(fp, sector * 512, SEEK_SET);
	if (ftell(fp) != sector * 512)
		return -1;
	fwrite(data, 512, 1, fp);
	if (ftell(fp) != (sector + 1) * 512)
		return -1;
	return 0;
}


int read_sector(int sector, uint8_t *data) {
	fseek(fp, sector * 512, SEEK_SET);
	if (ftell(fp) != sector * 512)
		return -1;
	fread(data, 512, 1, fp);
	if (ftell(fp) != (sector + 1) * 512)
		return -1;
	return 0;
}


int main(int argc, char **argv) {
	int fd, size, i;

	fp = fopen(argv[1], "r+");

	init_fat();
	fd = fat_open("/ARNE/HEJ.TXT", O_RDWR);
	size = fat_fsize(fd);
	fprintf(stderr, "fd=%i, size=%i\n", fd, size);
	for (i = 0; i < size / 512; i++)
		fat_read_sect(fd);
	fat_read_sect(fd);
	sector_buff[size & 0x1FF] = 0;
	fprintf(stderr, "Contents of last sector in file:\n%s", sector_buff);

	return 0;
}
