#include <stdio.h>

#define ABS(x)(x < 0? -x : x)
int main(int argc, char ** argv) {
	printf("%d\n", ABS(5 - 7));
	return 0;
}
