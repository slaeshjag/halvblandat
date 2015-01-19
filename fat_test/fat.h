#ifndef __FAT_H__
#define __FAT_H__

int init_fat();

#define	O_WRONLY		2
#define	O_RDONLY		1
#define	O_RDWR			(O_RDONLY | O_WRONLY)
extern char sector_buff[512];

#endif
