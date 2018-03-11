#include <stdio.h>
extern int calcSum_yf(int array[], int arrayLen);

int main() {
	int array[4] = {1,2,3,4};
	int arrayLength = 4;
	printf("%d\n", calcSum_yf(array, arrayLength));
	int arr2[6] = {-1,4,5,3,9,30};
	int arrayLength_2 = 6;
<<<<<<< HEAD
	printf("%d\n", calcSum_yf(arr2, arrayLength_2));
=======
	printf("%d\n", calcSum_lz(arr2, arrayLength_2));
>>>>>>> 5e8155133a9960f1f93755ec312645423200eb8c
}
