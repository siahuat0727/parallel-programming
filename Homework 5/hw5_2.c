#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <stdbool.h>
#include <assert.h>
#include <dirent.h>
#include <errno.h>

#define MAX_LENGTH_LINE 1000
#define MAX_LENGTH_PATH 200
#define MAX_LENGTH_WORD 50
#define MAX_KEYWORDS    100

struct key{
	char keyword[MAX_LENGTH_WORD];
	unsigned int count;
};
struct key KEYWORDS[MAX_KEYWORDS];
int TOTAL_KEY;

struct node_t{
	char data[MAX_LENGTH_LINE];
	struct node_t* next;
};
struct node_t *front, *rear;

// return time of unit msec
unsigned int get_time(){
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec*1000 + tv.tv_usec/1000;
}

bool queue_empty(){
	return front == NULL && rear == NULL;
}

void enqueue(const char* str){
	struct node_t *tmp = (struct node_t *)malloc(sizeof(struct node_t));
	// copy data
	assert(strlen(str) < MAX_LENGTH_LINE);
	strcpy(tmp->data, str);
	tmp->next = NULL;

	if(queue_empty()){
		// insert
		front = rear = tmp;
	}else{ 
		// check if expected
		assert(front != NULL);
		assert(rear != NULL);
		// insert
		rear->next = tmp;
		rear = tmp;
	}
}

void dequeue(){
	if(queue_empty()){
		puts("queue is empty");
	}else{
		// check if expected
		assert(front != NULL);
		assert(rear != NULL);
		// delete
		struct node_t* tmp = front;
		if(front == rear)
			front = rear = NULL;
		else
			front = front->next;
		free(tmp);
	}
}

char* queue_front(){
	return queue_empty() ? NULL : front->data;
}

void queue_print(){
	puts("print queue");
	for(struct node_t* iter = front; iter; iter = iter -> next)
		printf("%s",iter->data);
	puts("");
}

char* lower(char *str){
	char* tmp = str;
	while(*str != '\0')
		*(str++) |= 1<<5;
	return tmp;
}

void open_file(const char* path, FILE **fp){
	*fp = fopen(path, "r");
	if (*fp == NULL){ // sth wrong (never happend if succuess)
		perror(path);
		exit(EXIT_FAILURE);
	}
}

void read_keywords(const char* path){
	// open file
	FILE *fp;
	open_file(path, &fp);
	assert(fp != NULL);
	// store keywords
	for (TOTAL_KEY = 0; fscanf(fp, "%s", KEYWORDS[TOTAL_KEY].keyword) != EOF; ++TOTAL_KEY){
		if (TOTAL_KEY >= MAX_KEYWORDS){
			fprintf(stderr, "keywords more than expected, please adjust MAX_KEYWORDS");
			exit(EXIT_FAILURE);
		}
	}
}

// count all even if keywords are duplicate
void do_word(char* word){
	if(word == NULL)
		return;
	for(int i = 0; i < TOTAL_KEY; ++i){
		char tmp_keyword[MAX_LENGTH_WORD];
		strcpy(tmp_keyword, KEYWORDS[i].keyword);
		if(!strcmp(lower(tmp_keyword), lower(word)))
#pragma omp atomic
			KEYWORDS[i].count++;
	}
}

void do_line(char* line){
	if(line == NULL)
		return;
	char word[MAX_LENGTH_WORD];
	do{
		while(*line==' ' || *line=='\t')
			++line;
		if(sscanf(line, "%s", word) <= 0)
			break;
		line += strlen(word);
		do_word(word);
	}while(true);
}

bool receive(char *buf){
	if(queue_empty())
		return false;
	strcpy(buf, queue_front());
	dequeue();
	return true;
}

void print_keywords_count(){
	for(int i = 0; i < TOTAL_KEY; ++i)
		printf("%s\t: %u\n", KEYWORDS[i].keyword, KEYWORDS[i].count);
}

int main(int argc, const char**argv){
	char keywords_file[MAX_LENGTH_PATH] = "keywords.txt";
	char path_to_files[MAX_LENGTH_PATH] = "example";
	int num_thread = 6;
	if (argc == 4){
		num_thread = atoi(argv[1]);
		if(num_thread < 2){
			fprintf(stderr, "num of threads must greater or equal to 2");
			exit(EXIT_FAILURE);
		}
		memcpy(keywords_file, argv[2], strlen(argv[2]));
		memcpy(path_to_files, argv[3], strlen(argv[3]));
	}
	read_keywords(keywords_file);
	const int num_producer = num_thread / 2;
	int producer_done = 0;

	unsigned start_time = get_time();

	DIR* dir = opendir(path_to_files);
	if(errno == ENOENT){
		perror(path_to_files);
		exit(EXIT_FAILURE);
	}else if(errno == ENOTDIR){
		fprintf(stderr, "%s is not directory", path_to_files);
		exit(EXIT_FAILURE);
	}
	struct dirent *dp;
	char path[MAX_LENGTH_PATH];
#pragma omp parallel num_threads(num_thread) \
	default(none) shared(dir, path_to_files, producer_done) private(dp, path)
	{
		int thread_num = omp_get_thread_num();
		if(thread_num % 2){ // producer
			while(true){
#pragma omp critical(next_dir)
				dp = readdir(dir);
				if(dp == NULL)
					break;
				if(!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
					continue;
				sprintf(path, "%s/%s", path_to_files, dp->d_name);
				// open file
				FILE *fp;
				open_file(path, &fp);
				// insert all lines
				char buf[MAX_LENGTH_LINE] = "";
				while(fgets(buf, MAX_LENGTH_LINE-1, fp) != NULL){
#pragma omp critical(queue)
					enqueue(buf);
				}
			}
#pragma omp atomic
			producer_done++;
			// producer barrier
			while(producer_done != num_producer);
			if(thread_num == 1)
				closedir(dir);
		}else{ // consumer
			while(producer_done != num_producer || !queue_empty()){
				char buf[MAX_LENGTH_LINE] = "";
#pragma omp critical(queue)
				receive(buf);
				do_line(buf);
			}
		}
	}
	unsigned end_time = get_time();
	print_keywords_count();

	printf("total time used = %u ms\n", end_time - start_time);
	return 0;
}
