#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>
#include <omp.h>

//#define PRINT

int NUM_THREAD;

void Count_sort_serial(int a[], int n){
	int i ,j ,count;
	int *temp = malloc(n*sizeof(int));
	for(i = 0; i < n; ++i){
		count = 0;
		for(j = 0; j < n; ++j){
			if(a[j] < a[i])
				++count;
			else if(a[j] == a[i] && j < i)
				++count;
		}
		temp[count] = a[i];
	}
	memcpy(a, temp, n*sizeof(int));
	free(temp);
}

void Count_sort_parrallel(int a[], int n){
	int i ,j ,count;
	int *temp = malloc(n*sizeof(int));
#pragma omp parallel for num_threads(NUM_THREAD) \
	default(none) shared(a, n, temp) private(i, j, count)
	for(i = 0; i < n; ++i){
		count = 0;
		for(j = 0; j < n; ++j){
			if(a[j] < a[i])
				++count;
			else if(a[j] == a[i] && j < i)
				++count;
		}
		temp[count] = a[i];
	}
	memcpy(a, temp, n*sizeof(int));
	free(temp);
}


int cmp(const void* a, const void *b){
	return *(int*)a - *(int*)b;
}

void my_qsort(int a[], int n){
	qsort(a, n, sizeof(int), cmp);
}

// return time of unit msec
unsigned int get_time(){
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec*1000 + tv.tv_usec/1000;
}

unsigned int count_time(void (*f)(), int a[], int n){
	unsigned int start = get_time();
	f(a, n);
	unsigned int end = get_time();
	unsigned int total_msec = end - start;
#ifdef PRINT
	puts("");
	for(int i = 0; i < n; ++i)
		printf("%d\n", to_sort[i]);
#endif
	return total_msec;
}

// ./hw5_1 NUM_TO_SORT NUM_OF_THREADS
int main(int argc, const char** argv){
	srand(time(0));
	int n = 10000;
	NUM_THREAD = 4;
	if(argc == 3){
		n = atoi(argv[1]);
		if(n <= 0){
			fprintf(stderr, "n must > 0");
			exit(EXIT_FAILURE);
		}
		NUM_THREAD = atoi(argv[2]);
		if(NUM_THREAD <= 0){
			fprintf(stderr, "n must > 0");
			exit(EXIT_FAILURE);
		}
	}

	int *origin  = (int*)malloc(n * sizeof(int));
	int *to_sort = (int*)malloc(n * sizeof(int));
	for(int i = 0; i < n; ++i){
		origin[i] = rand();
#ifdef PRINT
		printf("%d\n", origin[i]);
#endif
	}

	memcpy(to_sort, origin, n * sizeof(int));
	unsigned int serial_count_sort = count_time(&Count_sort_serial, to_sort, n);

	memcpy(to_sort, origin, n * sizeof(int));
	unsigned int parallel_count_sort = count_time(&Count_sort_parrallel, to_sort, n);

	memcpy(to_sort, origin, n * sizeof(int));
	unsigned int serial_qsort = count_time(&my_qsort, to_sort, n);

	printf("serial count sort used = %u ms\n", serial_count_sort);
	printf("parallel count sort used = %u ms\n", parallel_count_sort);
	printf("serial qsort used = %u ms\n", serial_qsort);

	free(origin);
	free(to_sort);
	return 0;
}
