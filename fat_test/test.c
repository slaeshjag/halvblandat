#include "test.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "fat.h"

FILE *fp;

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
	fp = fopen(argv[1], "r+");

	init_fat();
	return 0;
}
