#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "fat.h"

#define	READ_WORD(x, n)		((uint16_t) ((uint16_t) x[n] | (((uint16_t) x[(n) + 1]) << 8)))
#define	READ_DWORD(x, n)	(((uint32_t) READ_WORD(x, n) | (((uint32_t) READ_WORD(x, (n) + 2)) << 16)))
uint32_t locate_record(const char *path, int *record_index);

char sector_buff[512];

enum FAT_TYPE {
	FAT_TYPE_FAT12,
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


static bool end_of_chain_mark(uint32_t cluster) {
	if (fat_state.type == FAT_TYPE_FAT12)
		return cluster >= 0xFF8 ? true : false;
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
		fat_state.type = FAT_TYPE_FAT12;
		fprintf(stderr, "FAT12 detected\n");
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
	fat_state.valid = true;

	tu32 = locate_record("/ARNE/HEJ.TXT", &i);
	fprintf(stderr, "Record for /ARNE/HEJ.TXT: %u:%i\n", tu32, i);
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


uint32_t next_cluster(uint32_t prev_cluster) {
	uint32_t cluster_pos, ut32, cluster_sec;
	int i;

	if (fat_state.type == FAT_TYPE_FAT12) {
		ut32 = 0;
		cluster_pos = prev_cluster / 2 * 3;
		cluster_sec = cluster_pos >> 9;
		cluster_pos &= 0x1FF;
		if (read_sector(cluster_sec, sector_buff) < 0) {
			fat_state.valid = false;
			return 0;
		}

		if (cluster_sec + 3 >= 512) {
			for (i = 0; cluster_pos + i < 512; i++)
				ut32 |= sector_buff[cluster_pos] << (i << 3);
			if (read_sector(cluster_sec + 1, sector_buff) < 0) {
				fat_state.valid = false;
				return 0;
			}
		} else
			i = 0;
		for (; i < 3; i++)
			ut32 |= sector_buff[cluster_pos] << (i << 3);
		if (prev_cluster & 1)
			cluster_sec = ut32 >> 12;
		else
			cluster_sec = ut32 & 0xFFF;
		if (cluster_sec >= 0xFF7)
			return 0;
	} else if (fat_state.type == FAT_TYPE_FAT16) {
		cluster_pos = prev_cluster << 1;
		cluster_sec = cluster_pos >> 9;
		cluster_pos &= 0x1FF;
		if (read_sector(cluster_sec, sector_buff) < 0) {
			fat_state.valid = false;
			return 0;
		}
			
		cluster_sec = READ_WORD(sector_buff, cluster_pos);
		if (cluster_sec >= 0xFFF7)
			return 0;
	} else {
		cluster_pos = prev_cluster << 2;
		cluster_sec = cluster_pos >> 9;
		cluster_pos &= 0x1FF;
		if (read_sector(cluster_sec, sector_buff) < 0) {
			fat_state.valid = false;
			return 0;
		}

		cluster_sec = READ_DWORD(sector_buff, cluster_pos) & 0x0FFFFFFF;
		if (cluster_sec >= 0x0FFFFFF7)
			return 0;
	}

	return (cluster_sec - 2) * fat_state.cluster_size + (fat_state.fat_pos + (fat_state.fat_size << 2));
}


uint32_t locate_record(const char *path, int *record_index) {
	char component[13], fatname[11];
	int i;
	uint32_t cur_sector;
	bool recurse = false;

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
		cur_sector = (READ_WORD(sector_buff, i * 32 + 20) << 16) | (READ_WORD(sector_buff, i * 32 + 26));
		cur_sector -= 2;
		cur_sector *= fat_state.cluster_size;
		cur_sector += fat_state.data_region;
	}
	strncpy(component, path, 13);
	for (i = 0; *path && *path != '/'; i++)
		path++;
	if (*path)
		path++;
	if (i < 13)
		component[i] = 0;
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
				if (!*sector_buff)	/* End of list */
					break;
				/* Deleted record, skip */
				continue;
			}

			if (!memcmp(fatname, &sector_buff[i * 32], 11)) {
				recurse = true;
				if (sector_buff[i * 32 + 11] & 0x10) {
					/* It's a directory.. */
				}
				goto found_component;
			}
		}

		if (cur_sector / fat_state.cluster_size != (cur_sector + 1) / fat_state.cluster_size) {
			cur_sector = next_cluster((cur_sector - fat_state.data_region) / fat_state.cluster_size + 2);
			if (!cur_sector)
				return 0;
			cur_sector -= 2;
			cur_sector *= fat_state.cluster_size;
			cur_sector += fat_state.data_region;
		} else
			cur_sector++;
	}
		
		
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
