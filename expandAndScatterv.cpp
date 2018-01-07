#include <stdio.h>
#include <string.h>
#include <mpi.h>
#include <math.h>

#define RAW 4
#define COL 5

int **a;
int **b;

int **alloc_int(int Y, int X )
{        
    int **temp = new int *[ Y ];
    int *temp2 = new int [ Y * X ];
    memset( temp, 0, sizeof( int ) * Y);
    memset( temp2, 0, sizeof( int ) * Y * X );

    for( int i = 0; i < Y; i++){
        temp[ i ] = &temp2[i*X];
    }

    return temp;
}

void expand_raws(int** dest, int** src)
{
    memcpy(dest[0], src[RAW-1], COL * sizeof(int) ); // expand first raw
    memcpy(dest[RAW+1], src[0], COL * sizeof(int) ); // expand last raw
}

void expandInt()
{
    b = alloc_int(RAW + 2, COL);
    memcpy(b[1], a[0], sizeof( int ) * RAW * COL);
    expand_raws(b, a);
}

void print_int(int** arr, int raw, int col){
    for(int i = 0; i < raw; ++i){
        for(int j = 0; j < col; ++j)
            printf("%2d ", arr[i][j]);
        putchar('\n');
    }
    putchar('\n');
}

int main(){
    int rank, size;     // for storing this process' rank, and the number of processes
    int sendcounts[] = {6,6,6,6};    // array describing how many elements to send to each process
    int displs[] = {0, 3, 7, 9};        // array describing the displacements where each segment begins

    a = alloc_int(RAW, COL);
    for(int i = 0; i < RAW; ++i)
        for(int j = 0; j < COL; ++j)
            a[i][j] = COL*i + j;
    print_int(a, RAW, COL);
    expandInt();
    print_int(b, RAW+2, COL);
}

