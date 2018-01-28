#include <stdio.h>
extern int calcSum_lz(int array[], int arrayLen);

int main() {
	int array[4] = {1,2,3,4};
	int arrayLength = 4;
	printf("%d\n", calcSum_lz(array, arrayLength));
}
