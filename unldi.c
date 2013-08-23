/* unldi.c - Steven Arnow <s@rdw.se>,  2013 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>


#define	DARNIT_FS_IMG_MAGIC	0x83B3661B
#define	DARNIT_FS_IMG_VERSION	0xBBA77ABC


typedef struct {
	unsigned int			magic;
	unsigned int			version;
	unsigned int			files;
} FILESYSTEM_IMAGE_HEADER;


typedef struct {
	char				name[128];
	unsigned int			pos;
	unsigned int			length;
	unsigned int			comp;
} FILESYSTEM_IMAGE_FILE;


unsigned int endian_swap(unsigned int src) {
	union {
		unsigned int 	i;
		unsigned char 	c[4];
	} a, b;

	a.i = 1;
	if (a.c[3])
		return src;
	a.i = src;
	b.c[3] = a.c[0];
	b.c[2] = a.c[1];
	b.c[1] = a.c[2];
	b.c[0] = a.c[3];

	return b.i;
}


void endian_swap_block(unsigned int *block, int ints) {
	int i;

	for (i = 0; i < ints; i++)
		block[i] = endian_swap(block[i]);
	return;
}


void make_dir(const char *dir) {
	mkdir(dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	return;
}


void write_file(const char *dir, FILE *fp, FILESYSTEM_IMAGE_FILE f) {
	char buff[2048], buff2[2048], buff3[2048], *str;
	int i;
	FILE *out;

	sprintf(buff, "%s", f.name);
	*buff2 = 0;
	sprintf(buff2, "%s", dir);
	make_dir(dir);

	str = strtok(buff, "/");

	for (;;) {
		sprintf(buff3, "%s/%s", buff2, str);
		strcpy(buff2, buff3);
		str = strtok(NULL, "/");
		if (!str)
			break;
		make_dir(buff2);
	}

	fprintf(stderr, "Opening %s\n", buff2);
	if (!(out = fopen(buff2, "wb")))
		return;
	for (i = 0; i < f.length; )
		i += fwrite(buff, 1, fread(buff, 1, 
			(f.length - i < 2048) ? f.length - i : 2048, fp), out);
	fclose(out);
	
	return;
}


int main(int argc, char **argv) {
	FILE *fp;
	int i;
	off_t pos;
	FILESYSTEM_IMAGE_HEADER h;
	FILESYSTEM_IMAGE_FILE f;

	if (argc <3) {
		fprintf(stderr, "Usage: %s <input> <target dir>\n", argv[0]);
		return -1;
	}

	if (!(fp = fopen(argv[1], "rb"))) {
		fprintf(stderr, "Unable to open %i\n", argv[1]);
		return -1;
	}

	fread(&h, sizeof(h), 1, fp);
	endian_swap_block((void *) &h, sizeof(h) / 4);

	if (h.magic != DARNIT_FS_IMG_MAGIC || h.version != DARNIT_FS_IMG_VERSION) {
		fprintf(stderr, "Invalid LDI file\n");
		return -1;
	}

	for (i = 0; i < h.files; i++) {
		fread(&f, sizeof(f), 1, fp);
		endian_swap_block((void *) &f.pos, 3);
		pos = ftell(fp);
		fseek(fp, f.pos + sizeof(FILESYSTEM_IMAGE_HEADER) + sizeof(FILESYSTEM_IMAGE_FILE) * h.files, SEEK_SET);
		write_file(argv[2], fp, f);
		fseek(fp, pos, SEEK_SET);
	}


	return 0;
}
