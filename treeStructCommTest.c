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

	int count = 0;
	int to_skip = 0;
	int sender_bit = 1;
	while(sender_bit != comm_sz){
		if(id & to_skip) // true if the process already sent sth
			break;

		if(id & sender_bit){ // the process which sender_bit = 1 is the sender for this loop 
			int receiver = id^sender_bit;
			printf("I'm %d, i want to send to %d\n", id, receiver);
			MPI_Send(&count, 1, MPI_INT, receiver, 0, MPI_COMM_WORLD);
		}else{
			int sender = id|sender_bit;
			printf("I'm %d, i want to receive from %d\n", id, sender);
			MPI_Recv(&count, 1, MPI_INT, sender,  0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		}

		to_skip |= sender_bit;
		sender_bit <<= 1;
	}
	MPI_Finalize();
	return 0;
}
