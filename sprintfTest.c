#include <stdio.h>
#include <string.h>

#define SIZE 10

char str[100];

// store result in global variable str
void arrayToString(int arr[], int n){
	int index = 0;
	for(int i = 0; i < n; ++i)	{
		index += sprintf(&str[index], "%d", arr[i]);
		str[index++] = ' ';
	}
	str[index] = '\0';
}

int main(){
	int input[SIZE];
	for(int i = 0; i < SIZE; ++i)
		input[i] = 2*i + 1;
	arrayToString(input, SIZE);
	puts(str);
	return 0;
}
