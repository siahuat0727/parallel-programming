#include <stdio.h>
#include <string.h>

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
    memcpy(dest[0], src[3-1], 3 * sizeof(int) ); // expand first raw
    memcpy(dest[3+1], src[0], 3 * sizeof(int) ); // expand last raw
}

void expandInt()
{
    b = alloc_int(3 + 2, 3);
    memcpy(b[1], a[0], sizeof( int ) * 3 * 3);
    expand_raws(b, a);
}

void print_int(int** arr, int n){
    for(int i = 0; i < n; ++i){
        for(int j = 0; j < 3; ++j)
            printf("%d ", arr[i][j]);
        putchar('\n');
    }
    putchar('\n');
}

int main(){
    a = alloc_int(3, 3);
    for(int i = 0; i < 3; ++i)
        for(int j = 0; j < 3; ++j)
            a[i][j] = 3*i + j;
    print_int(a, 3);
    expandInt();
    print_int(a, 3);
    print_int(b, 5);
}

