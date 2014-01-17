#include "linux/input.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>


const char *key_names[256] = {
	[1]	= 	"Escape",
	[2]	=	"1",
	[3]	=	"2",
	[4]	=	"3",
	[5]	=	"4",
	[6]	=	"5",
	[7]	=	"6",
	[8]	=	"7",
	[9]	=	"8",
	[10]	=	"9",
	[11]	=	"0",
	[12]	=	"+",
	[13]	=	"´",
	[14]	=	"Backspace",
	[15]	=	"Tab",
	[16]	=	"Q",
	[17]	=	"W",
	[18]	=	"E",
	[19]	=	"R",
	[20]	=	"T",
	[21]	=	"Y",
	[22]	=	"U",
	[23]	=	"I",
	[24]	=	"O",
	[25]	=	"P",
	[26]	=	"Å",
	[27]	=	"¨",
	[28]	=	"Enter",
	[29]	=	"LCTRL",
	[30]	=	"A",
	[31]	=	"S",
	[32]	=	"D",
	[33]	=	"F",
	[34]	=	"G",
	[35]	=	"H",
	[36]	=	"J",
	[37]	=	"K",
	[38]	=	"L",
	[39]	=	"Ö",
	[40]	=	"Ä",
	[41]	=	"'",
	[42]	=	"LSHIFT",
	[43]	=	"<",
	[44]	=	"Z",
	[45]	=	"X",
	[46]	=	"C",
	[47]	=	"V",
	[48]	=	"B",
	[49]	=	"N",
	[50]	=	"M",
	[51]	=	",",
	[52]	=	".",
	[53]	=	"-",
	[54]	=	"RSHIFT",
	[56]	=	"LALT",
	[57]	=	"SPACE",
};


int key_usage[256];
int key_stats[256];
int key_total = 0;
char keybuff[16];
time_t t;


const char *key_name(int key) {
	if (key >= 255) {
		sprintf(keybuff, "<%i>", key);
		return keybuff;
	}

	if (!key_names[key]) {
		sprintf(keybuff, "<%i>", key);
		return keybuff;
	}

	return key_names[key];
}


void sort_used() {
	int i, j, n;

	for (i = 0; i < 256; i++)
		key_stats[i] = i;
	for (i = 0; i < 256; i++)
		for (j = i; j > 0; j--)
			if (key_usage[key_stats[j]] > key_usage[key_stats[j - 1]]) {
				n = key_stats[j];
				key_stats[j] = key_stats[j - 1];
				key_stats[j - 1] = n;
			}
	return;
}
				


void quit(int signum) {
	int key, i;

	sort_used();
	fprintf(stderr, "A total of %i keys were pressed\n", key_total);
	fprintf(stderr, "%lli minutes has passed\n", (time(NULL) - t) / 60);

	fprintf(stderr, "Most used keys:\n");
	for (i = 0; i < 5; i++) {
		key = key_stats[i];
		fprintf(stderr, "Key \"%s\" was pressed %i times (%f%%)\n", key_name(key), key_usage[key], (float) key_usage[key] / key_total * 100);
	}

	fprintf(stderr, "ALT was pressed %i times (%f%%)\n", key_usage[KEY_LEFTALT], (float) key_usage[KEY_LEFTALT] / key_total * 100);
	
	if (signum == SIGINT)
		exit(0);
	return;
}


int main(int argc, char **argv) {
	int fd, i;
	struct input_event ev;

	signal(SIGINT, quit);
	signal(SIGUSR1, quit);
	t = time(NULL);
	for (i = 0; i < 256; i++)
		key_usage[i] = 0;

	if ((fd = open(argv[1], O_RDONLY)) < 0)
		return 0;
	
	for (;;) {
		read(fd, &ev, sizeof(ev));

		if (ev.type != EV_KEY)
			continue;
		if (ev.value)
			continue;
		if (ev.code > 255)
			continue;
		key_total++;
		key_usage[ev.code]++;
	}

	return 0;
}
