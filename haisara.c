/* haisara.c - Steven Arnow <s@rdw.se>,  2013 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdint.h>


void beizer(int p0x, int p0y, int p1x, int p1y, int p2x, int p2y, int color) {
	int i;
	int t, x, n;

	p1x -= p0x;
	p1y -= p0y;
	p2x -= p0x;
	p2y -= p0y;

	for (i = 0; i < 2048; i++) {
		t = 2048 - i;
		t <<= 1;
		t *= i;
		t >>= 11;
		t *= p1x;
		n = t;

		t = i;
		t *= i;
		t >>= 11;
		t *= p2x;
		t += n;
		t >>= 11;

		fprintf(stderr, "X=%x\n", t);
	}

	return;
}



int main(int argc, char **argv) {
	beizer(0, 0, 127, 0, 64, 64, 512);

	return 0;
}
