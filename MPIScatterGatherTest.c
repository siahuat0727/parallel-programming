#include <stdio.h>
#include <mpi.h>
#include <string.h>

#define ARR_SIZE 32
#define STR_SIZE 500

void arrayToString(int arr[], int n, char str[]){
	int index = 0;
	for(int i = 0; i < n; ++i)	{
		index += sprintf(&str[index], "%d", arr[i]);
		str[index++] = ' ';
	}
	str[index] = '\0';
}

int main (int argc, char *argv[]) {
	// Initialize
	int id;
	int comm_sz;
	MPI_Init(NULL, NULL);
	MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
	MPI_Comm_rank(MPI_COMM_WORLD, &id);

	int arr[ARR_SIZE];
	if(id == 0)
		for(int i = 0; i < ARR_SIZE; ++i)
			arr[i] = i;
	int local_n = ARR_SIZE / comm_sz;
	int receive_arr[local_n];
	MPI_Scatter(arr, local_n, MPI_INT, receive_arr, local_n, MPI_INT, 0, MPI_COMM_WORLD);

	char str[STR_SIZE];
	arrayToString(receive_arr, local_n, str);
	printf("I'm %d, I received: %s\n", id, str);

	for(int i = 0; i < local_n; ++i)
		receive_arr[i] *= 2;

	MPI_Gather(receive_arr, local_n, MPI_INT, arr, local_n, MPI_INT, 0, MPI_COMM_WORLD);
	
	if(id == 0){
		arrayToString(arr, ARR_SIZE, str);
		printf("I'm %d, my arr is now: %s\n", id, str);
	}

	MPI_Finalize();
	return 0;
}
