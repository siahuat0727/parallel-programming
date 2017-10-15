/* circuitSatifiability.c solves the Circuit Satisfiability
 *  Problem using a brute-force sequential solution.
 *
 *   The particular circuit being tested is "wired" into the
 *   logic of function 'checkCircuit'. All combinations of
 *   inputs that satisfy the circuit are printed.
 *
 *   16-bit version by Michael J. Quinn, Sept 2002.
 *   Extended to 32 bits by Joel C. Adams, Sept 2013.
 */

#include <stdio.h>     // printf()
#include <limits.h>    // UINT_MAX
#include <mpi.h>
#include <stdlib.h>
#include <time.h>

typedef long long int ll;

double randDouble(){
	return (double)rand() / RAND_MAX * 2 - 1; 
}

// for tesing only
void testRandDouble(){
	int i = 100;
	while(i--)
		printf("%f\n", randDouble());
}

int main (int argc, char *argv[]) {
	int id;           /* process id */
	int comm_sz;

	MPI_Init(NULL, NULL);
	MPI_Comm_rank(MPI_COMM_WORLD, &id);
	MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);

	ll total_tosses = 0;
	ll local_total = 0;
	ll num_in_circle = 0;
	if(id == 0)
		scanf("%lld", &total_tosses);

	MPI_Bcast(&total_tosses, 1, MPI_LONG_LONG, 0, MPI_COMM_WORLD);

	if(id == 0)
		local_total = total_tosses - (comm_sz - 1)*(total_tosses / comm_sz); 
	else
		local_total = total_tosses / comm_sz;	

	srand(time(NULL) + id);

	// calculate total time
	double startTime = 0.0, totalTime = 0.0;
	startTime = MPI_Wtime();

	while(local_total--){
		double x = randDouble();
		double y = randDouble();
		if( x*x + y*y <= 1)
			num_in_circle++;
	}

	for(int tmp = comm_sz, next_tmp; tmp > 1; tmp = next_tmp){
		next_tmp = (tmp+1) >> 1;
		if(id >= tmp)
			continue;
		else if(id >= next_tmp){ /* sender */
			int receiver = id - next_tmp;
			MPI_Send(&num_in_circle, 1, MPI_LONG_LONG, receiver, 0, MPI_COMM_WORLD);
		}else if(id < tmp >> 1){ /* receiver */
			int sender = id + next_tmp;
			ll receive_value;
			MPI_Recv(&receive_value, 1, MPI_LONG_LONG, sender, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			num_in_circle += receive_value;
		}
	}

	totalTime = MPI_Wtime() - startTime;

	if(id == 0){
		printf("estimate %f\n", 4*num_in_circle / (double)total_tosses);
		printf("Process %d finished in time %f secs.\n", id, totalTime);
	}

	MPI_Finalize();
	return 0;
}
