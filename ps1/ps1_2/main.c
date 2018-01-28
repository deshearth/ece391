#include <stdio.h>
extern int calcSum_lz(int array[], int arrayLen);

int main() {
	int array[4] = {1,2,3,4};
	int arrayLength = 4;
	printf("%d\n", calcSum_lz(array, arrayLength));
	int arr2[6] = {-1,4,5,3,9,30};
	int arrayLength_2 = 6;
	printf("%d\n", calcSum_lz(arr2, arrayLength_2));
}
