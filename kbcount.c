#include "linux/input.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>


int key_usage[256];
int key_total = 0;
time_t t;


void quit(int signum) {
	fprintf(stderr, "A total of %i keys were pressed\n", key_total);
	fprintf(stderr, "%lli minutes has passed\n", (time(NULL) - t) / 60);
	fprintf(stderr, "ALT was pressed %i times (%f%%)\n", key_usage[KEY_LEFTALT], (float) key_usage[KEY_LEFTALT] / key_total * 100);
	exit(0);
	return;
}


int main(int argc, char **argv) {
	int fd, i;
	struct input_event ev;

	signal(SIGINT, quit);
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
