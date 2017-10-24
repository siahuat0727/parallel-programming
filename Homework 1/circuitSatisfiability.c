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

int checkCircuit (int, long);

int main (int argc, char *argv[]) 
{
	long i = 0;           /* loop variable (64 bits) */
	int id = 0;           /* process id */
	int comm_sz = 0;      /* total number of processes */
	int count = 0;        /* number of solutions */


	MPI_Init(NULL, NULL);
	MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
	MPI_Comm_rank(MPI_COMM_WORLD, &id);

	// calculate total time
	double startTime = 0.0, totalTime = 0.0;
	startTime = MPI_Wtime();

	// set the interval of each rank
	long n = ((long)UINT_MAX + 1) / comm_sz;
	long local_start = id * n;
	long local_end = local_start + n;

	// start counting
	for (i = local_start; i < local_end; i++) 
	{
		count += checkCircuit (id, i);
	}

	int to_skip = 0;
	int sender_bit = 1;

 	// Tree-structed communication, work only for comm_sz = 2^n, where n = 0, 1, 2, 3, ...
	while(sender_bit != comm_sz)
	{
		if(id & to_skip) // true if the process has already sent sth
			break;

		if(id & sender_bit)
		{ // the process which sender_bit = 1 is the sender for this loop 
			int receiver = id^sender_bit;
			MPI_Send(&count, 1, MPI_INT, receiver, 0, MPI_COMM_WORLD);
		}
		else
		{ // receiver
			int receive_value;
			int sender = id|sender_bit;
			MPI_Recv(&receive_value, 1, MPI_INT, sender,  0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			count += receive_value;
		}

		to_skip |= sender_bit;
		sender_bit <<= 1;
	}

	if(id == 0)
	{
		printf("%2d core(s):\n", comm_sz);
		printf("A total of %d solutions were found.\n", count);
		totalTime = MPI_Wtime() - startTime;
		printf("Process finished in time %f secs.\n\n", totalTime);
	}

	MPI_Finalize();
	return 0;
}

/* EXTRACT_BIT is a macro that extracts the ith bit of number n.
 *
 * parameters: n, a number;
 *             i, the position of the bit we want to know.
 *
 * return: 1 if 'i'th bit of 'n' is 1; 0 otherwise 
 */

#define EXTRACT_BIT(n,i) ( (n & (1<<i) ) ? 1 : 0)


/* checkCircuit() checks the circuit for a given input.
 * parameters: id, the id of the process checking;
 *             bits, the (long) rep. of the input being checked.
 *
 * output: the binary rep. of bits if the circuit outputs 1
 * return: 1 if the circuit outputs 1; 0 otherwise.
 */

#define SIZE 32

int checkCircuit (int id, long bits) 
{
	int v[SIZE];        /* Each element is a bit of bits */
	int i;

	for (i = 0; i < SIZE; i++)
		v[i] = EXTRACT_BIT(bits,i);

	if ( ( (v[0] || v[1]) && (!v[1] || !v[3]) && (v[2] || v[3])
				&& (!v[3] || !v[4]) && (v[4] || !v[5])
				&& (v[5] || !v[6]) && (v[5] || v[6])
				&& (v[6] || !v[15]) && (v[7] || !v[8])
				&& (!v[7] || !v[13]) && (v[8] || v[9])
				&& (v[8] || !v[9]) && (!v[9] || !v[10])
				&& (v[9] || v[11]) && (v[10] || v[11])
				&& (v[12] || v[13]) && (v[13] || !v[14])
				&& (v[14] || v[15]) )
			||
			( (v[16] || v[17]) && (!v[17] || !v[19]) && (v[18] || v[19])
			  && (!v[19] || !v[20]) && (v[20] || !v[21])
			  && (v[21] || !v[22]) && (v[21] || v[22])
			  && (v[22] || !v[31]) && (v[23] || !v[24])
			  && (!v[23] || !v[29]) && (v[24] || v[25])
			  && (v[24] || !v[25]) && (!v[25] || !v[26])
			  && (v[25] || v[27]) && (v[26] || v[27])
			  && (v[28] || v[29]) && (v[29] || !v[30])
			  && (v[30] || v[31]) ) )
	{
		/*
		   printf ("%d) %d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d \n", id,
		   v[31],v[30],v[29],v[28],v[27],v[26],v[25],v[24],v[23],v[22],
		   v[21],v[20],v[19],v[18],v[17],v[16],v[15],v[14],v[13],v[12],
		   v[11],v[10],v[9],v[8],v[7],v[6],v[5],v[4],v[3],v[2],v[1],v[0]);
		 */
		fflush (stdout);
		return 1;
	}
   	else
   	{
		return 0;
	}
}

