#include <stdio.h>


void func2(void *ptr) {
	goto *ptr;
	return;
}


void func1(int i) {

	blah:
	fprintf(stderr, "Hello %i!\n", i);
	func2(&&blah);

	return;
}


int main(int argc, char **argv) {
	func1(1);
	return 0;
}
