#ifndef __FAT_H__
#define __FAT_H__

int init_fat();

#define	O_WRONLY		2
#define	O_RDONLY		1
#define	O_RDWR			(O_RDONLY | O_WRONLY)
extern uint8_t sector_buff[512];

struct FATDirList {
	char		filename[13];
	uint8_t		attrib;
	uint8_t		padding[2];
} __attribute__((packed));

#endif
