#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <assert.h>
#include <sys/time.h>
#define MAX_CITY 50
#define MAX_ANT 5000
#define PATH_LENGTH 20




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

unsigned int get_time(){
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec*1000 + tv.tv_usec/1000;
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

int main(){
	double evap_rate = 0.3;
	double alpha = 1;
	double beta = 1;
	int Q = 10;
	int num_ant = 400;
	int dis[MAX_CITY][MAX_CITY];

	char ques_file[PATH_LENGTH] = "Readme.txt";
	FILE *fp;
	open_file(&fp, ques_file);
	char file_name[PATH_LENGTH];
	int num_city;
	int minimal_tour;
	while(fscanf(fp, "%s is a set of %d cities, from TSPLIB. The minimal tour has length %d.", file_name, &num_city, &minimal_tour) == 3){
		int shortest_path_length = 0x3f3f3f3f;
		int short_path[MAX_CITY+1];
		to_lower(file_name);
		strcat(file_name, "_d.txt");
		FILE *city_fp;
		open_file(&city_fp, file_name);

		read_distance_table(num_city, dis, file_name, city_fp);

		double pheromone[MAX_CITY][MAX_CITY];
		memset(pheromone, 0, sizeof(pheromone));
		double delta[MAX_CITY][MAX_CITY];
		bool visited[MAX_ANT][MAX_CITY];
		int path_length[MAX_ANT];
		int path[MAX_ANT][MAX_CITY+1];

		for(int nc = 0; nc < 500; ++nc){
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
						else{
							prob[i] = pow(pheromone[cur_city][i], alpha) * pow(1/(double)dis[cur_city][i], beta);
						}
						total_prob += prob[i];
					}
					// generate random double between 0 and total_prob
					double random = (double)rand()/RAND_MAX * total_prob;
					// choose next city base on possibility
					int next_city = -1;
					for(int i = 0; i < num_city; ++i){
						if(visited[k][i] == true)
							continue;
						random -= prob[i];
						if (random <= 1e-7){
							next_city = i;
							break;
						}
					}
					if(n == num_city-1)
						next_city = path[k][0];
					assert(next_city != -1);
					path_length[k] += dis[cur_city][next_city];
					path[k][n+1] = next_city;
					visited[k][next_city] = true;
				}
			}
			for(int k = 0; k < num_ant; ++k){
				if(path_length[k] < shortest_path_length){
					shortest_path_length = path_length[k];
					memcpy(short_path, path[k], (num_city+1)*sizeof(int));
					printf("shortest = %d\n", shortest_path_length);
				}	
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
			if(shortest_path_length == minimal_tour){
				puts("found best path!!!!!!!!");
				break;
			}
		}
		for(int i = 0; i <= num_city; ++i){
			if(i)
				printf(" -> ");
			printf("%d", short_path[i]);
		}
		puts("");
	}
}
