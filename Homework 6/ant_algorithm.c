#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <assert.h>
#include <sys/time.h>
#include <mpi.h>

#define MAX_CITY 50
#define MAX_ANT 5000
#define PATH_LENGTH 20
#define INF 0x3f3f3f3f

#define PRINT // delete if don't want to show process

struct data{
	int shortest_path_length;
	int rank;
};

void open_file(FILE **fp, const char* path)
{
	*fp = fopen(path, "r");
	if(!*fp){
		perror(path);
		exit(EXIT_FAILURE);
	}
}

void to_lower(char *str){
	while(*str){
		if(*str >= 'A' && *str <= 'Z')
			*str |= 1 << 5;
		++str;
	}
}

void read_distance_table(int num_city, int dis[][MAX_CITY], const char* file_name, FILE *city_fp){
	for (int i = 0; i < num_city; ++i){
		for (int j = 0; j < num_city; ++j){
			if(fscanf(city_fp, "%d", &dis[i][j]) != 1){
				perror(file_name);
				exit(EXIT_FAILURE);
			}
		}
	}
	char dummy[5];
	assert(fscanf(city_fp, "%s", dummy) <= 0); // should be nothing left
}

void init_pheromone(double pheromone[][MAX_CITY], int num_city)
{
	for(int i = 0; i < num_city; ++i)
		for(int j = 0; j < num_city; ++j)
			pheromone[i][j] = 1e-8;
}

int main(int argc, char *argv[]){
	MPI_Init(&argc, &argv);

	int rank, size;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	// parameter that can be adjust
	double evap_rate = 0.3;
	double alpha = 1;
	double beta = 1;
	int Q = 10;
	int num_ant = 500;
	int num_thread = 8;
	int num_simulate = 50;
	int exchange_cycle = 10;

	int dis[MAX_CITY][MAX_CITY];

	char ques_file[PATH_LENGTH] = "ques.txt";
	FILE *fp;
	open_file(&fp, ques_file);
	char file_name[PATH_LENGTH];
	int num_city;
	int minimal_tour;
	while(fscanf(fp, "%s is a set of %d cities, from TSPLIB. The minimal tour has length %d.", file_name, &num_city, &minimal_tour) == 3){
		int shortest_path[MAX_CITY+1];
		to_lower(file_name);
		strcat(file_name, "_d.txt");
		FILE *city_fp;
		open_file(&city_fp, file_name);

		read_distance_table(num_city, dis, file_name, city_fp);
		struct data global_data = {.shortest_path_length = INF};
		struct data local_process= {.rank = rank, .shortest_path_length = INF};

#pragma omp parallel num_threads(num_thread)\
		default(none)\
		shared(shortest_path, alpha, beta, num_city, num_ant, evap_rate, Q, dis, minimal_tour, exchange_cycle, local_process, rank, num_simulate)
		{
			int thread_num = omp_get_thread_num();
			srand(rank * num_thread + thread_num); // set difference seed for thread and reproducible
			double pheromone[MAX_CITY][MAX_CITY];
			memset(pheromone, 0, sizeof(pheromone));
			init_pheromone(pheromone, num_city);
			double delta[MAX_CITY][MAX_CITY];
			bool visited[MAX_ANT][MAX_CITY];
			int path_length[MAX_ANT];
			int path[MAX_ANT][MAX_CITY+1];
			int thread_shortest_path_length = INF;
			int thread_shortest_path[MAX_CITY+1];

			for(int nc = 0; nc < num_simulate; ++nc){
				// init
				memset(visited, false, sizeof(visited));
				memset(delta, 0, sizeof(delta));
				memset(path_length, 0, sizeof(path_length));
				memset(path, 0, sizeof(path));

				// put the ants at random city
				for (int k = 0; k < num_ant; ++k){
					path[k][0] = rand() % num_city;
					visited[k][path[k][0]] = true;
				}

				// simulate
				for (int n = 0; n < num_city; ++n){
					for (int k = 0; k < num_ant; ++k){
						// calculate possibility for next city
						double total_prob = 0;
						double prob[MAX_CITY];
						int cur_city = path[k][n];
						for(int i = 0; i < num_city; ++i){
							if(visited[k][i] == true || dis[cur_city][i] == 0)
								prob[i] = 0;
							else
								prob[i] = pow(pheromone[cur_city][i], alpha) * pow(1/(double)dis[cur_city][i], beta);
							total_prob += prob[i];
						}
						// generate random double between 0 and total_prob
						double random = (double)rand()/RAND_MAX * total_prob;
						// choose next city base on possibility
						int next_city;
						for(next_city = 0; next_city < num_city; ++next_city){
							if(visited[k][next_city] == true)
								continue;
							random -= prob[next_city];
							if (random <= 1e-7)
								break;
						}
						if(n == num_city-1)
							next_city = path[k][0];
						assert(next_city != num_city);
						path_length[k] += dis[cur_city][next_city];
						path[k][n+1] = next_city;
						visited[k][next_city] = true;
					}
				}
				for(int k = 0; k < num_ant; ++k){
					if(path_length[k] < thread_shortest_path_length){
						thread_shortest_path_length = path_length[k];
						memcpy(thread_shortest_path, path[k], (num_city+1)*sizeof(thread_shortest_path[0]));
					}	
				}

				if(nc % exchange_cycle == 0){
					if(thread_shortest_path_length < local_process.shortest_path_length){
#pragma omp critical
						if(thread_shortest_path_length < local_process.shortest_path_length){
							local_process.shortest_path_length = thread_shortest_path_length;
							memcpy(shortest_path, thread_shortest_path, (num_city+1) * sizeof(shortest_path[0]));
						}
#ifdef PRINT
						printf("%d :shorter path length = %d\n", rank, local_process.shortest_path_length);
#endif
					}
					//#pragma omp barrier
					thread_shortest_path_length = local_process.shortest_path_length;
					memcpy(thread_shortest_path, shortest_path, (num_city+1) * sizeof(shortest_path[0]));
				}
				// calculate new pheromone
				for (int k = 0; k < num_ant; ++k){
					int city_i = path[k][0];
					for (int j = 1; j <= num_city; ++j){
						int city_j = path[k][j];
						delta[city_i][city_j] += (double)Q / path_length[k];
						city_i = city_j;
					}
					if(city_i != path[k][0]){
						printf("k = %d\n", k);
						for(int i = 0; i < num_city+1; ++i)
							printf("%d\t", path[k][i]);
						puts("");

					}
					assert(city_i == path[k][0]);
				}
				// update pheromone
				for (int i = 0; i < num_city; ++i){
					for (int j = 0; j < num_city; ++j){
						pheromone[i][j] = (1 - evap_rate)*pheromone[i][j] + delta[i][j];
					}
				}
				if(local_process.shortest_path_length == minimal_tour)
					break;
			}
		}
		MPI_Barrier(MPI_COMM_WORLD);
		// see who found shortest path
		MPI_Allreduce(&local_process, &global_data, 1, MPI_2INT, MPI_MINLOC, MPI_COMM_WORLD);
		// send result to process 0
		if(global_data.rank != 0){
			if(rank == global_data.rank)
				MPI_Send(shortest_path, num_city+1, MPI_INT, 0, 0, MPI_COMM_WORLD);
			if(rank == 0)
				MPI_Recv(shortest_path, num_city+1, MPI_INT, global_data.rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		}
		// process 0 print result
		if(rank == 0){
			printf("file: %s\n", file_name);
			printf("found short path length = %d\n", global_data.shortest_path_length);
			for(int i = 0; i <= num_city; ++i)
				printf("%s%d", i ? " -> " : "", shortest_path[i]);
			puts("\n");
		}
	}

	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Finalize();
	return 0;
}
