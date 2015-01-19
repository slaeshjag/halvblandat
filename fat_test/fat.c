#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "fat.h"

#define	MAX_FD_OPEN		4
#define	READ_WORD(x, n)		((uint16_t) ((uint16_t) x[(n)] | (((uint16_t) x[(n) + 1]) << 8)))
#define	READ_DWORD(x, n)	read_dword(x, n)
#define	WRITE_WORD(x, n, w)	((x)[(n)] = (w & 0xFF), (x)[(n) + 1] = ((w) >> 8) & 0xFF)
#define	WRITE_DWORD(x, n, dw)	(WRITE_WORD(x, n, dw & 0xFFFF), WRITE_WORD((x), (n) + 2, (dw) >> 16))
#define GET_ENTRY_CLUSTER(e)	(READ_WORD(sector_buff, e * 32 + 20) << 16) | (READ_WORD(sector_buff, e * 32 + 26))

uint32_t read_dword(uint8_t *buff, int byte) {
	uint32_t dw;

	dw = READ_WORD(buff, byte);
	dw |= (READ_WORD(buff, byte + 2) << 16);
	return dw;
}

uint32_t locate_record(const char *path, int *record_index);

char sector_buff[512];

enum FAT_TYPE {
	FAT_TYPE_FAT16,
	FAT_TYPE_FAT32,
};


struct {
	bool		valid;
	uint8_t		cluster_size;
	uint32_t	clusters;
	uint32_t	fat_size;
	uint32_t	fat_pos;
	uint32_t	root_dir_pos;
	uint32_t	data_region;
	enum FAT_TYPE	type;
} fat_state;

struct FATFileDescriptor {
	int32_t		key;
	uint32_t	entry_sector;
	uint32_t	entry_index;
	uint32_t	first_cluster;
	uint32_t	current_cluster;
	uint32_t	fpos;
	uint32_t	file_size;
	bool		write;
};

static struct FATFileDescriptor fat_fd[MAX_FD_OPEN];
static int32_t fat_key = 0;

static bool end_of_chain_mark(uint32_t cluster) {
	if (fat_state.type == FAT_TYPE_FAT16)
		return cluster >= 0xFFF8 ? true : false;
	return cluster >= 0x0FFFFFF8 ? true : false;
}


int init_fat() {
	uint8_t *data = sector_buff;
	uint32_t *u32;
	uint16_t *u16;
	uint16_t reserved_sectors;
	uint8_t *u8;
	uint8_t tu8;
	uint32_t tu32;
	int err, i, j;

	if ((err = read_sector(0, data) < 0)) {
		fat_state.valid = false;
		return err;
	}

	fat_state.cluster_size = data[13];
	reserved_sectors = READ_WORD(data, 14);

	u8 = &data[16];
	if (*u8 != 2) {
		fprintf(stderr, "Unsupported FAT: %i FAT:s in filesystem, only 2 supported\n", *u8);
		return -1;
	}
	
	if (READ_WORD(data, 19)) {
		fat_state.clusters = READ_WORD(data, 19) / fat_state.cluster_size;
	} else {
		fat_state.clusters = READ_DWORD(data, 32) / fat_state.cluster_size;
	}

	if (fat_state.clusters < 4085) {
		fprintf(stderr, "FAT12 unsupported\n");
		fprintf(stderr, "%i clusters\n", fat_state.clusters);
		return -1;
	} else if (fat_state.clusters < 65525) {
		fat_state.type = FAT_TYPE_FAT16;
		fprintf(stderr, "FAT16 detected\n");
	} else {
		fat_state.type = FAT_TYPE_FAT32;
		fprintf(stderr, "FAT32 detected\n");
	}

	if (fat_state.type <= FAT_TYPE_FAT16)
		tu8 = data[38];
	else
		tu8 = data[66];
	if (tu8 != 0x28 && tu8 != 0x29) {
		fprintf(stderr, "FAT signature check failed\n");
		return -1;
	}

	if (fat_state.type <= FAT_TYPE_FAT16) {
		fat_state.fat_size = READ_WORD(data, 22);
	} else {
		fat_state.fat_size = READ_DWORD(data, 36);
	}
	
	fat_state.fat_pos = reserved_sectors;
	fat_state.root_dir_pos = fat_state.fat_pos + fat_state.fat_size * 2;
	if (fat_state.type == FAT_TYPE_FAT32)
		fat_state.root_dir_pos = fat_state.fat_pos + fat_state.cluster_size * READ_DWORD(data, 44);
	fat_state.data_region = fat_state.root_dir_pos + READ_WORD(data, 17) * 32 / 512;

	for (i = 0; i < MAX_FD_OPEN; i++)
		fat_fd[i].key = -1;
	fat_state.valid = true;

	return 0;
}


void fname_to_fatname(char *name, char *fname) {
	int i, j;

	for (i = 0; i < 8 && name[i] != '.' && name[i]; i++)
		fname[i] = name[i];
	if (i < 8 && name[i]) {
		for (j = i; j < 8; j++)
			fname[j] = ' ';
		i++;
		for (; j < 11 && name[i]; j++, i++)
			fname[j] = name[i];
		for (; j < 11; j++)
			fname[j] = ' ';
	} else {
		for (; i < 11; i++)
			fname[i] = ' ';
	}

	return;
}


uint32_t cluster_to_sector(uint32_t cluster) {
	if (!cluster)
		return 0;
	return (cluster - 2) * fat_state.cluster_size + fat_state.data_region;
}

uint32_t sector_to_cluster(uint32_t sector) {
	if (!sector)
		return 0;
	return (sector - fat_state.data_region) / fat_state.cluster_size + 2;
}


uint32_t next_cluster(uint32_t prev_cluster) {
	uint32_t cluster_pos, ut32, cluster_sec;
	int i;

	if (fat_state.type == FAT_TYPE_FAT16) {
		cluster_pos = prev_cluster << 1;
		cluster_sec = cluster_pos >> 9;
		cluster_pos &= 0x1FF;
		cluster_sec += fat_state.fat_pos;
		if (read_sector(cluster_sec, sector_buff) < 0) {
			fat_state.valid = false;
			return 0;
		}
			
		cluster_sec = READ_WORD(sector_buff, cluster_pos);
	} else {
		cluster_pos = prev_cluster << 2;
		cluster_sec = cluster_pos >> 9;
		cluster_pos &= 0x1FF;
		cluster_sec += fat_state.fat_pos;
		if (read_sector(cluster_sec, sector_buff) < 0) {
			fat_state.valid = false;
			return 0;
		}

		cluster_sec = READ_DWORD(sector_buff, cluster_pos) & 0x0FFFFFFF;
	}

	if (end_of_chain_mark(cluster_sec))
		return 0;

	return cluster_sec;
}


uint32_t locate_record(const char *path, int *record_index) {
	char component[13], fatname[11];
	int i;
	uint32_t cur_sector;
	bool recurse = false, file = false;

	cur_sector = fat_state.root_dir_pos;

	found_component:
	while (*path == '/')
		path++;
	if (!*path) {
		if (!recurse)
			return 0;
		else {
			*record_index = i;
			return cur_sector;
		}
	} else if (recurse) {
		cur_sector = GET_ENTRY_CLUSTER(i);
		cur_sector = cluster_to_sector(cur_sector);
	}
	strncpy(component, path, 13);
	for (i = 0; *path && *path != '/'; i++)
		path++;
	if (*path)
		path++;
	if (i < 13)
		component[i] = 0;
	if (file && *path)	/* A non-directory mid-path */
		return 0;
	fname_to_fatname(component, fatname);
	for (;;) {
		if (read_sector(cur_sector, sector_buff) < 0) {
			fat_state.valid = false;
			return 0;
		}

		for (i = 0; i < 16; i++) {
			if ((sector_buff[i * 32 + 11] & 0xF) == 0xF)	/* Long filename entry */
				continue;
			if (!sector_buff[i * 32 + 11]) {
				if (!sector_buff[i * 32])	/* End of list */
					return 0;
				/* Deleted record, skip */
				continue;
			}

			if (!memcmp(fatname, &sector_buff[i * 32], 11)) {
				recurse = true;
				file = (sector_buff[i * 32 + 11] & 0x10) ? false : true;
				goto found_component;
			}
		}

		if (cur_sector / fat_state.cluster_size != (cur_sector + 1) / fat_state.cluster_size) {
			cur_sector = cluster_to_sector(next_cluster(sector_to_cluster(cur_sector)));
			if (!cur_sector)
				return 0;
		} else
			cur_sector++;
	}
}


uint32_t alloc_cluster(uint32_t entry_sector, uint32_t entry_index, uint32_t old_cluster) {
	int i, j;
	uint32_t cluster;

	for (i = 0; i < fat_state.fat_size; i++) {
		if (read_sector(fat_state.fat_pos + i, sector_buff) < 0) {
			fat_state.valid = false;
			return 0;
		}

		if (fat_state.type == FAT_TYPE_FAT16) {
			for (j = 0; j < 512; j+=2)
				if (!READ_WORD(sector_buff, j)) {
					cluster = i * 256 + j/2;
					WRITE_WORD(sector_buff, j, 0xFFFF);
					write_sector(fat_state.fat_pos + i, sector_buff);
					write_sector(fat_state.fat_pos + fat_state.fat_size + i, sector_buff);
					if (old_cluster) {
						read_sector(fat_state.fat_pos + old_cluster / 256);
						WRITE_WORD(sector_buff, (old_cluster & 0xFF) << 1, cluster);
						write_sector(fat_state.fat_pos + i, sector_buff);
						if (write_sector(fat_state.fat_pos + fat_state.fat_size + i, sector_buff) < 0) {
							fat_state.valid = false;
							return 0;
						}
					}

					goto allocated;
				}
		} else {
			for (j = 0; j < 512; j += 4)
				if (!READ_DWORD(sector_buff, j)) {
					cluster = i * 128 + j / 4;
					WRITE_DWORD(sector_buff, j, 0x0FFFFFFF);
					write_sector(fat_state.fat_pos + i, sector_buff);
					write_sector(fat_state.fat_pos + fat_state.fat_size + i, sector_buff);
					if (old_cluster) {
						read_sector(fat_state.fat_pos + old_cluster / 128);
						WRITE_DWORD(sector_buff, (old_cluster & 0x7F) << 2, cluster);
						write_sector(fat_state.fat_pos + i, sector_buff);
						if (write_sector(fat_state.fat_pos + fat_state.fat_size + i, sector_buff) < 0) {
							fat_state.valid = false;
							return 0;
						}
					}

					goto allocated;
				}
		}
	}
	
	allocated:
	if (!old_cluster) {
		read_sector(entry_sector, sector_buff);
		WRITE_WORD(sector_buff, entry_index * 32 + 20, cluster >> 16);
		WRITE_WORD(sector_buff, entry_index * 32 + 26, cluster);
		write_sector(entry_sector, sector_buff);
	}

	return cluster;
}


int fat_open(const char *path, int flags) {
	int key, i, index;
	uint32_t sector;

	if (!fat_state.valid)
		return -1;

	for (i = 0; i < MAX_FD_OPEN; i++)
		if (fat_fd[i].key < 0)
			break;
	if (i == MAX_FD_OPEN)
		return -1;
	
	if (!(sector = locate_record(path, &index)))
		return -1;
	if (read_sector(sector, sector_buff) < 0) {
		fat_state.valid = false;
		return -1;
	}

	if (sector_buff[index * 32 + 11] & 0x10)
		/* Don't allow opening a directory */
		return -1;
	fat_fd[i].write = flags&O_WRONLY?true:false;
	fat_fd[i].entry_sector = sector;
	fat_fd[i].entry_index = index;
	fat_fd[i].first_cluster = GET_ENTRY_CLUSTER(index);
	if (fat_fd[i].first_cluster == 0) {
		if (fat_fd[i].write)
			fat_fd[i].first_cluster = alloc_cluster(fat_fd[i].entry_sector, fat_fd[i].entry_index, 0);
		fat_fd[i].file_size = 0;
	} else {
		fat_fd[i].file_size = read_dword(sector_buff, index * 32 + 28);
	}

	fat_fd[i].current_cluster = fat_fd[i].first_cluster;
	fat_fd[i].fpos = 0;
	fat_fd[i].key = fat_key++;
	return fat_fd[i].key;
}


uint32_t fat_fsize(int fd) {
	int i;
	
	if (fd < 0)
		return 0;
	for (i = 0; i < MAX_FD_OPEN; i++)
		if (fat_fd[i].key == fd)
			break;
	if (i == MAX_FD_OPEN)
		return 0;
	return fat_fd[i].file_size;
}


uint32_t fat_ftell(int fd) {
	int i;

	if (fd < 0)
		return 0;
	for (i = 0; i < MAX_FD_OPEN; i++)
		if (fat_fd[i].key == fd)
			break;
	if (i == MAX_FD_OPEN)
		return 0;
	return fat_fd[i].fpos;
}


bool fat_read_sect(int fd) {
	int i;
	uint32_t old_cluster, sector;
	
	if (fd < 0)
		return false;
	for (i = 0; i < MAX_FD_OPEN; i++)
		if (fat_fd[i].key == fd)
			break;
	if (i == MAX_FD_OPEN)
		return false;
	if (!fat_fd[i].current_cluster)
		return false;
	if (fat_fd[i].fpos == fat_fd[i].file_size)
		return false;
	sector = cluster_to_sector(fat_fd[i].current_cluster) + ((fat_fd[i].fpos >> 9) % fat_state.cluster_size);
	
	fat_fd[i].fpos += 512;
	if (fat_fd[i].file_size < fat_fd[i].fpos)
		fat_fd[i].fpos = fat_fd[i].file_size & (~0x1FF);
	if (!(fat_fd[i].fpos % (fat_state.cluster_size * 512))) {
		old_cluster = fat_fd[i].current_cluster;
		fat_fd[i].current_cluster = next_cluster(fat_fd[i].current_cluster);
		if (!fat_fd[i].current_cluster && fat_fd[i].write)
			fat_fd[i].current_cluster = alloc_cluster(fat_fd[i].entry_sector, fat_fd[i].entry_index, old_cluster);
	}
	
	if (read_sector(sector, sector_buff) < 0) {
		fat_state.valid = false;
		return false;
	}

	return true;
}


bool fat_write_sect(int fd) {
	int i;
	uint32_t old_cluster, sector;

	if (fd < 0)
		return false;
	for (i = 0; i < MAX_FD_OPEN; i++)
		if (fat_fd[i].key == fd)
			break;
	if (i == MAX_FD_OPEN)
		return false;
	if (!fat_fd[i].write)
		return false;
	if (!fat_fd[i].current_cluster)
		return false;
	sector = cluster_to_sector(fat_fd[i].current_cluster);
	sector += (fat_fd[i].fpos >> 9) % fat_state.cluster_size;
	write_sector(sector, sector_buff);
	fat_fd[i].fpos += 512;
	if (!((fat_fd[i].fpos >> 9) % fat_state.cluster_size)) {
		old_cluster = fat_fd[i].current_cluster;
		fat_fd[i].current_cluster = next_cluster(fat_fd[i].current_cluster);
		if (!fat_fd[i].current_cluster && fat_fd[i].write)
			fat_fd[i].current_cluster = alloc_cluster(fat_fd[i].entry_sector, fat_fd[i].entry_index, old_cluster);
	}

	read_sector(fat_fd[i].entry_sector, sector_buff);
	WRITE_DWORD(sector_buff, fat_fd[i].entry_index * 32 + 28, fat_fd[i].fpos);
	write_sector(fat_fd[i].entry_sector, sector_buff);
}


#if 0
	if ((err = read_sector(fat_state.root_dir_pos, data) < 0)) {
		fprintf(stderr, "Unable to read root directory\n");
		fat_state.valid = false;
		return err;
	}
	
	for (j = 0; j < 16; j++) {
		if ((data[j*32 + 11] & 0x8))
			continue;
		if (!data[j*32 + 11])
			break;
		fprintf(stderr, "File name: ");
		for (i = 0; i < 11; i++)
			fprintf(stderr, "%c", data[j*32 + i]);
		fprintf(stderr, "\n");
		if (!(data[j*32 + 11] & 0x10))
			fprintf(stderr, "This is not a directory\n");
		fprintf(stderr, "Creation date: %i-%i-%i\n", (READ_WORD(data, j*32 + 16) >> 9) + 1980, (READ_WORD(data, j * 32 + 16) >> 5) & 0xF, (READ_WORD(data, j * 32 + 16) & 0x1F));
		tu32 = (READ_WORD(data, j * 32 + 20) << 16) | (READ_WORD(data, j * 32 + 26));
		fprintf(stderr, "Data cluster: 0x%X\n", tu32);
	}

	fprintf(stderr, "End of directory\n");
	fname_to_fatname
}
#endif
