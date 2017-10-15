/*
 * Work only for comm_sz = 2^n, n = 0, 1, 2, ...
 */
#include <stdio.h>
#include <mpi.h>

int main (int argc, char *argv[]) {
	int id;
	int comm_sz;
	MPI_Init(NULL, NULL);
	MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
	MPI_Comm_rank(MPI_COMM_WORLD, &id);

	int count = id;
	int to_skip = 0;
	int sender_bit = 1;
	for(int tmp = 1; tmp < comm_sz; tmp <<= 1){
		if(id < tmp){ // sender
			int receiver = id + tmp;
			if(receiver < comm_sz){
				printf("I'm %d, i want to send to %d\n", id, id + tmp);
				MPI_Send(&count, sizeof count, MPI_INT, id + tmp, 0, MPI_COMM_WORLD);
			}
		}else if(id < 2*tmp){ // receiver
			printf("I'm %d, i want to receive from %d\n", id, id - tmp);
			MPI_Recv(&count, sizeof count, MPI_INT, id - tmp, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		}else // wait
			continue;
	}
	MPI_Finalize();
	return 0;
}
