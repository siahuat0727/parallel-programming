/*   findPi.c estimate the value of pi
 *   Using "Monte Carlo" method 
 */

#include <stdio.h>
#include <limits.h>
#include <mpi.h>
#include <stdlib.h>
#include <time.h>

typedef long long int ll;

double randDouble()
{
	return (double)rand() / RAND_MAX * 2 - 1; 
}

// for tesing only
void testRandDouble()
{
	int i = 100;
	while(i--)
		printf("%f\n", randDouble());
}

int main (int argc, char *argv[]) 
{
	int id;           /* process id */
	int comm_sz;      /* total number of processes */

	MPI_Init(NULL, NULL);
	MPI_Comm_rank(MPI_COMM_WORLD, &id);
	MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);

	ll total_tosses = 0;
	ll local_total = 0;
	ll num_in_circle = 0;

	// process 0 read the total number of tosses
	if(id == 0)
		scanf("%lld", &total_tosses);

	// broadcast to other processes
	MPI_Bcast(&total_tosses, 1, MPI_LONG_LONG, 0, MPI_COMM_WORLD);

	// count the local total number of tosses of each process
	if(id == 0)
		local_total = total_tosses - (comm_sz - 1)*(total_tosses / comm_sz); 
	else
		local_total = total_tosses / comm_sz;	

	// set different random seed for each process
	srand(time(NULL) + id);

	// calculate total time
	double startTime = 0.0, totalTime = 0.0;
	startTime = MPI_Wtime();

	// Monte Carlo method
	while(local_total--)
	{
		double x = randDouble();
		double y = randDouble();
		if( x*x + y*y <= 1)
			num_in_circle++;
	}

	// Alternative Tree-structed communication, process 0 collect the global sum
	for(int tmp = comm_sz, next_tmp; tmp > 1; tmp = next_tmp)
	{ // tmp: the total number of processes which haven't passed its result
		next_tmp = (tmp+1) >> 1;
		if(id >= tmp)
			continue;
		else if(id >= next_tmp)
		{ /* sender */
			int receiver = id - next_tmp;
			MPI_Send(&num_in_circle, 1, MPI_LONG_LONG, receiver, 0, MPI_COMM_WORLD);
		}
		else if(id < tmp >> 1)
		{ /* receiver */
			int sender = id + next_tmp;
			ll receive_value;
			MPI_Recv(&receive_value, 1, MPI_LONG_LONG, sender, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			num_in_circle += receive_value;
		}
	}

	// calculate total time
	totalTime = MPI_Wtime() - startTime;

	// process 0 print the result
	if(id == 0)
	{
		double pi_estimate = 4*num_in_circle / (double)total_tosses;
		printf("%2d core(s):\n", comm_sz);
		printf("After %lld tosses, the approximate value of pi is %f\n", total_tosses, pi_estimate);
		printf("Process finished in time %f secs.\n\n", totalTime);
	}

	MPI_Finalize();
	return 0;
}
