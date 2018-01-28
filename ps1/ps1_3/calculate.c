#include <stdio.h>
#include <stdint.h>

int calculate(uint32_t operation, int arg1, int arg2) {
	if (operation > 3) 
		return 0xECE391;
	else if (operation == 0) {
		return *((int*)(arg2 * 8 + arg1 - 4)) + arg2;
		//return 0;
	}
	else if (operation == 1) {
		if (arg1 + 5 < arg2) return arg1;
		else return arg2;
	}
	else if (operation == 2) {
		return arg2 >> (arg1 & 0x00000011);
	}
	else if (operation == 3) {
		return arg2 + arg1 * 2 + 27;
	}
}

int main() {
	printf("%d\n", calculate(0,1,2));
}
