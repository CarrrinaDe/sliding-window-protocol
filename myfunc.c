#include <stdio.h>
#include "mylib.h"

int xor(char *p) {
	int sum = 0, c;
 	for (c = sizeof(int); c < sizeof(pkt); c++)
		sum ^= p[c];
    return sum;
}
