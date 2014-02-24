/* chs_calculator.c - Steven Arnow <s@rdw.se>,  2014 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>


int main(int argc, char **argv) {
	int bytes;
	int sectors;
	int cylinders;
	int heads;

	bytes = atoi(argv[1]);
	sectors = bytes / 512;


	return 0;
}
