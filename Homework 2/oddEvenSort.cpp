#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int cmp(const void* a, const void* b){
	return *(int*)a - *(int*)b;
}

int get_partner(int phase, int rank, int size){
	int partner = -1;
	if(phase % 2 == 0) // even phase
		if(rank % 2  != 0) // odd rank
			partner = rank - 1;
		else // even rank
			partner = rank + 1;
	else // odd phase
		if(rank % 2 != 0) // odd rank
			partner = rank + 1;
		else // even rank
			partner = rank - 1;
	if(partner == -1 || partner == size)
		partner = -1;

	return partner;
}

void Merge_low(int* my_keys, int* recv_keys, int* temp_keys, int my_num, int recv_num){
	int m_i, r_i, t_i;
	m_i = r_i = t_i = 0;
	while(t_i < my_num)
		if(r_i == recv_num) // when merge_low, my_num maybe more than recv_num (prevent array out of index)
			temp_keys[t_i++] = my_keys[m_i++];
		else if(my_keys[m_i] <= recv_keys[r_i])
			temp_keys[t_i++] = my_keys[m_i++];
		else
			temp_keys[t_i++] = recv_keys[r_i++];
	for(m_i = 0; m_i < my_num; ++m_i)
		my_keys[m_i] = temp_keys[m_i];

}

void Merge_high(int* my_keys, int* recv_keys, int* temp_keys, int my_num, int recv_num){
	int m_i, r_i, t_i;
	m_i = t_i = my_num - 1;
	r_i = recv_num - 1;
	while(t_i >= 0)
		if(my_keys[m_i] >= recv_keys[r_i])
			temp_keys[t_i--] = my_keys[m_i--];
		else
			temp_keys[t_i--] = recv_keys[r_i--];
	for(m_i = 0; m_i < my_num; ++m_i)
		my_keys[m_i] = temp_keys[m_i];

}

int main(int argc,char *argv[])
{

	MPI_Init(&argc,&argv);
	double startwtime = MPI_Wtime();

	int rank, size;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	int total_num;
	if(rank == 0){
		printf("Please enter the total number of keys to sort: ");
		scanf("%d", &total_num);
	}

	// broadcast total_num to all processes
	MPI_Bcast(&total_num, 1, MPI_INT, 0, MPI_COMM_WORLD);

	// calculate the total # of random number to be generated for each process
	int *send_counts = (int*)malloc(size * sizeof(int));
	int *displaces = (int*)malloc(size * sizeof(int));
	int initial_send_count = total_num / size;
	int rem = total_num % size;
	int sum = 0;
	for(int i = 0; i < size; ++i){
		send_counts[i] = initial_send_count;
		if(rem-- > 0)
			++send_counts[i];
		displaces[i] = sum;
		sum += send_counts[i];
	}

	// allocate memory
	int* arr = (int *)malloc(send_counts[rank] * sizeof(int));
	int* buf = (int *)malloc(send_counts[rank] * sizeof(int));
	int* temp = (int *)malloc(send_counts[rank] * sizeof(int));

	// set different random seed
	srand(time(NULL) + rank);

	// generate random number
	for(int i = 0; i < send_counts[rank]; ++i)
		arr[i] = rand();

	// sort the local random number
	qsort(arr, send_counts[rank], sizeof(int), cmp);

	// allocate memory for process 0 to print the result
	int* results = NULL;
	if(rank == 0)
		results = (int*)malloc(total_num * sizeof(int));

	// all process send data to process 0
	MPI_Gatherv(arr, send_counts[rank], MPI_INT, results, send_counts, displaces, MPI_INT, 0, MPI_COMM_WORLD);

	// process 0 prints local list
	if(rank == 0){
		for(int i = 0; i < size; ++i){
			printf("process %2d:", i);
			for(int j = 0; j < send_counts[i]; ++j)
				printf(" %10d", results[displaces[i] + j]);
			puts("");
		}
	}
	
	// odd-even sort
	for(int phase = 0; phase < size; ++phase){
		int partner = get_partner(phase, rank, size);
		if(partner != -1){
			MPI_Sendrecv(arr, send_counts[rank], MPI_INT, partner, 0, buf, send_counts[partner], MPI_INT, partner, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			if(rank < partner)
				Merge_low(arr, buf, temp, send_counts[rank], send_counts[partner]);
			else
				Merge_high(arr, buf, temp, send_counts[rank], send_counts[partner]);
		}
	}

	// all process send data to process 0
	MPI_Gatherv(arr, send_counts[rank], MPI_INT, results, send_counts, displaces, MPI_INT, 0, MPI_COMM_WORLD);

	// print result
	if(rank == 0){
		printf("result:");
		for(int i = 0; i < total_num; ++i)
			printf(" %d", results[i]);
		puts("");
	}

	// free memory allocated
//	free(arr);
//	free(buf);
//		free(temp);
//	if(rank == 0)
//		free(results);

	MPI_Finalize();
	return 0;
}
