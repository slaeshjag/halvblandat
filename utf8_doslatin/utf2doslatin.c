#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "lookup.h"

unsigned char *char_lookup(const unsigned char *str) {
	int i;

	for (i = 0; replace_arr[i*2]; i++)
		if (!strcmp(str, replace_arr[i * 2]))
			return replace_arr[i * 2 + 1];
	if (*str < 0x7F)
		return str;
	return "?";
}


int main() {
	unsigned char buff[8];
	int left, tmp;

	while (!feof(stdin)) {
		buff[0] = fgetc(stdin);
		if (feof(stdin))
			return 0;
		if (buff[0] < 0x80) {
			buff[1] = 0;
			fprintf(stdout, "%s", char_lookup(buff));
			continue;
		} else {
			if ((buff[0] & 0xC0) == 0x80) {	/* Bad continuation byte */
				fputc('?', stdout);
				continue;
			}

			tmp = buff[0] << 1;
			for (left = 0; tmp & 0x80; tmp <<= 1, left++);
			for (tmp = 0; tmp < left; tmp++)
				buff[1 + tmp] = fgetc(stdin);
			buff[1 + tmp] = 0;
			fprintf(stdout, "%s", char_lookup(buff));
		}
		buff[0] = 0;
	}
			
	return 0;
}
