#include <stdio.h>
#include <mpi.h>

int main (int argc, char *argv[]) {
	int id;
	int comm_sz;
	MPI_Init(NULL, NULL);
	MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
	MPI_Comm_rank(MPI_COMM_WORLD, &id);

	int count[2];
	count[0] = id;
	count[1] = 2*id;
	int sum[2];
	sum[0] = sum[1] = 0;
	MPI_Allreduce(&count, &sum, 2, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
	printf("I'm %d, my sum1 is %d\n", id, sum[0]);
	printf("I'm %d, my sum2 is %d\n", id, sum[1]);
	MPI_Finalize();
	return 0;
}
