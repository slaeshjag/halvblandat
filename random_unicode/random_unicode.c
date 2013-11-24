/* random_unicode.c - Steven Arnow <s@rdw.se>,  2013 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "utf8.h"


int main(int argc, char **argv) {
	FILE *fp;
	int i;
	unsigned int n;
	char buf[5];

	/* If it doesn't exist, you're not using a real OS */
	fp = fopen("/dev/urandom", "rb");
	fprintf(stdout, "Dagens lösenord: ");
	for (i = 0; i < 20; i++) {
		fread(&n, 4, 1, fp);
		n = n % UTF8_CHAR_LIMIT;
		buf[utf8Encode(n, buf, 5)] = 0;
		fprintf(stdout, "%s", buf);
	}
	fprintf(stdout, "\n");

	return 0;
}
