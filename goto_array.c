/* goto_array.c - Steven Arnow <s@rdw.se>,  2014 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>


int main(int argc, char **argv) {
	void *array[3] = {
		&&hello,
		&&hello2,
		&&quit,
	};
	void *p;
	int i = -1;
	back:
		i++;
		goto *array[i];

	hello:
		fprintf(stderr, "Hello!\n");
		goto back;
	hello2:
		fprintf(stderr, "Hello2!\n");
		goto back;
	quit:
	return 0;
}
