#include "test.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "fat.h"

FILE *fp;

int write_sector_call(int sector, uint8_t *data) {
	fseek(fp, sector * 512, SEEK_SET);
	if (ftell(fp) != sector * 512)
		return -1;
	fwrite(data, 512, 1, fp);
	if (ftell(fp) != (sector + 1) * 512)
		return -1;
	return 0;
}


int read_sector_call(int sector, uint8_t *data) {
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

	delete_file("/ARNE/LOLOL.ASD");
	delete_file("/ARNE");
	create_file(0, "TESTCASE", 0x10);
	create_file("/TESTCASE", "ARNE", 0x10);
	create_file("/TESTCASE/ARNE", "TEST.BIN", 0x0);

	fd = fat_open("/TESTCASE/ARNE/TEST.BIN", O_RDWR);
	size = fat_fsize(fd);
	fprintf(stderr, "fd=%i, size=%i\n", fd, size);
	for (i = 0; i < 129; i++) {
		memset(sector_buff, i, 512);
		fat_write_sect(fd);
	}

	fat_close(fd);
	delete_file("/TESTCASE/ARNE/TEST.BIN");
	delete_file("/TESTCASE/ARNE");
	delete_file("/TESTCASE");
	create_file(0, "ARNE", 0x10);
	create_file("/ARNE", "LOLOL.ASD", 0x10);

	return 0;
}
