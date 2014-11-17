#include <stdio.h>
#include <stdlib.h>

#include "lookup.h"

/* Assume single-byte encoding as ingoing. If you want to adapt this to work
** with something even sillier, like Shift-JIS, look elsewhere...		*/


unsigned char *char_lookup(const unsigned char *str) {
	int i;

	for (i = 0; replace_arr[i*2]; i++)
		if (!strcmp(str, replace_arr[i * 2 + 1]))
			return replace_arr[i * 2];
	if (*str < 0x7F)
		return str;
	return "?";
}


int main() {
	unsigned char buff[2];
	buff[1] = 0;

	while (!feof(stdin)) {
		buff[0] = fgetc(stdin);
		if (feof(stdin))
			return 0;
		fprintf(stdout, "%s", char_lookup(buff));
	}

	return 0;
}
