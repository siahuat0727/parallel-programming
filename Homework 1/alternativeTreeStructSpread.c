/*
 * Compile: mpiicc [filename] -std=c99
 * Execute: mpiexec -n k ./a.out , where k = 1, 2, 3, ...
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
	for(int tmp = comm_sz, next_tmp; tmp > 1; tmp = next_tmp){
		next_tmp = (tmp+1) >> 1;
		if(id >= tmp)
			continue;
		else if(id >= next_tmp){ /* sender */
			int receiver = id - next_tmp;
			printf("I'm %d, i want to send to %d\n", id, receiver);
			MPI_Send(&count, 1, MPI_INT, receiver, 0, MPI_COMM_WORLD);
		}else if(id < tmp >> 1){ /* receiver */
			int sender = id + next_tmp;
			printf("I'm %d, i want to receive from %d\n", id, sender);
			MPI_Recv(&count, 1, MPI_INT, sender, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		}
	}

	for(int tmp = 1; tmp < comm_sz; tmp <<= 1){
		if(id < tmp){ /* sender */
			int receiver = id + tmp;
			if(receiver < comm_sz){ /* check if receiver exist */
				printf("I'm %d, i want to send to %d\n", id, receiver);
				MPI_Send(&count, 1, MPI_INT, receiver, 0, MPI_COMM_WORLD);
			}
		}else if(id < 2*tmp){ /* receiver */
			int sender = id - tmp;
			if(id < comm_sz){ /* check if this id(receiver) exist */
				printf("I'm %d, i want to receive from %d\n", id, sender);
				MPI_Recv(&count, 1, MPI_INT, sender, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			}
		}
	}
	MPI_Finalize();
	return 0;
}
