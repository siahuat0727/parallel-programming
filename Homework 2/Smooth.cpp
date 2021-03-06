#include <mpi.h>
#include <iostream>
#include <string>
#include <fstream>
#include <stdio.h>
#include <cstring>
#include <cstdlib>
#include "bmp.h"

using namespace std;

//定義平滑運算的次數
#define NSmooth 1000

/*********************************************************/
/*變數宣告：                                             */
/*  bmpHeader    ： BMP檔的標頭                          */
/*  bmpInfo      ： BMP檔的資訊                          */
/*  **BMPSaveData： 儲存要被寫入的像素資料               */
/*  **BMPData    ： 暫時儲存要被寫入的像素資料           */
/*********************************************************/
BMPHEADER bmpHeader;                        
BMPINFO bmpInfo;
RGBTRIPLE **BMPSaveData = NULL;                                               
RGBTRIPLE **BMPPartialSaveData = NULL;                                                   
RGBTRIPLE **BMPPartialData = NULL;                                                   

/*********************************************************/
/*函數宣告：                                             */
/*  readBMP    ： 讀取圖檔，並把像素資料儲存在BMPSaveData*/
/*  saveBMP    ： 寫入圖檔，並把像素資料BMPSaveData寫入  */
/*  swap       ： 交換二個指標                           */
/*  **alloc_memory： 動態分配一個Y * X矩陣               */
/*********************************************************/
int readBMP( char *fileName);        //read file
int saveBMP( char *fileName);        //save file
void swap(RGBTRIPLE *a, RGBTRIPLE *b);
RGBTRIPLE **alloc_memory( int Y, int X );        //allocate memory

int main(int argc,char *argv[])
{
	/*********************************************************/
	/*變數宣告：                                             */
	/*  *infileName  ： 讀取檔名                             */
	/*  *outfileName ： 寫入檔名                             */
	/*  startwtime   ： 記錄開始時間                         */
	/*  endwtime     ： 記錄結束時間                         */
	/*********************************************************/
	char *infileName = "input.bmp";
	char *outfileName = "output2.bmp";
	double startwtime = 0.0;

	MPI_Init(&argc,&argv);

	// to calculate total time
	startwtime = MPI_Wtime();

	int rank, size;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	// read image
	if ( rank == 0 && readBMP( infileName) )
		cout << "Read file successfully!!" << endl;
	else if(rank == 0) 
		cout << "Read file fails!!" << endl;

	// broadcast image info to all processes
	MPI_Bcast(&bmpInfo.biHeight, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(&bmpInfo.biWidth, 1, MPI_INT, 0, MPI_COMM_WORLD);

	// define a data type for 1 raw
	MPI_Datatype BMP_RAW;
	MPI_Type_contiguous(sizeof(RGBTRIPLE)*bmpInfo.biWidth, MPI_CHAR, &BMP_RAW);
	MPI_Type_commit(&BMP_RAW);

	// calculate the # of raws to do for each process
	int *send_counts = (int*)malloc(size * sizeof(int));
	int *displaces = (int*)malloc(size * sizeof(int));
	int initial_send_count = bmpInfo.biHeight / size;
	int rem = bmpInfo.biHeight % size;
	int sum = 0;
	for(int i = 0; i < size; ++i){
		send_counts[i] = initial_send_count;
		if(rem-- > 0)
			++send_counts[i];
		displaces[i] = sum;
		sum += send_counts[i];
	}

	BMPPartialSaveData = alloc_memory(send_counts[rank]+2 , bmpInfo.biWidth);

	// divide image into SIZE parts and send to each process
	MPI_Scatterv(rank==0 ? BMPSaveData[0] : NULL, send_counts, displaces, BMP_RAW, BMPPartialSaveData[1], send_counts[rank], BMP_RAW, 0, MPI_COMM_WORLD);

	//動態分配記憶體給暫存空間
	BMPPartialData = alloc_memory(send_counts[rank]+2 , bmpInfo.biWidth);
	MPI_Request send_requests[2], recv_requests[2];
	MPI_Status send_statuses[2], recv_statuses[2];

	//進行多次的平滑運算
	for(int count = 0; count < NSmooth ; count ++){
		// send to up
		MPI_Isend((char* )BMPPartialSaveData[1], 1, BMP_RAW, (rank+size-1) % size, 0, MPI_COMM_WORLD, &send_requests[0]);

		// receive from down
		MPI_Irecv((char* )BMPPartialSaveData[send_counts[rank]+1], 1, BMP_RAW, (rank+1) % size, 0, MPI_COMM_WORLD, &recv_requests[0]);

		// send to down
		MPI_Isend((char* )BMPPartialSaveData[send_counts[rank]], 1, BMP_RAW, (rank+1) % size, 1, MPI_COMM_WORLD , &send_requests[1]);

		// receive from up
		MPI_Irecv((char* )BMPPartialSaveData[0], 1, BMP_RAW, (rank+size-1) % size, 1, MPI_COMM_WORLD, &recv_requests[1]);

		// wait until sended and received
		if(MPI_Waitall(2, send_requests, send_statuses) == MPI_ERR_IN_STATUS)
			cout << "no!!" << endl;
		if(MPI_Waitall(2, recv_requests, recv_statuses) == MPI_ERR_IN_STATUS)
			cout << "no2!!" << endl;

		//把像素資料與暫存指標做交換
		swap(BMPPartialSaveData,BMPPartialData);

		//進行平滑運算
		for(int i = 1; i<=send_counts[rank] ; i++)
			for(int j =0; j<bmpInfo.biWidth ; j++){
				/*********************************************************/
				/*設定上下左右像素的位置                                 */
				/*********************************************************/
				int Top   = i - 1;
				int Down  = i + 1;
				int Left  = j>0 ? j-1 : bmpInfo.biWidth-1;
				int Right = j<bmpInfo.biWidth-1 ? j+1 : 0;
				/*********************************************************/
				/*與上下左右像素做平均，並四捨五入                       */
				/*********************************************************/
				BMPPartialSaveData[i][j].rgbBlue =  (double) (BMPPartialData[i][j].rgbBlue+BMPPartialData[Top][j].rgbBlue+BMPPartialData[Down][j].rgbBlue+BMPPartialData[i][Left].rgbBlue+BMPPartialData[i][Right].rgbBlue)/5+0.5;
				BMPPartialSaveData[i][j].rgbGreen =  (double) (BMPPartialData[i][j].rgbGreen+BMPPartialData[Top][j].rgbGreen+BMPPartialData[Down][j].rgbGreen+BMPPartialData[i][Left].rgbGreen+BMPPartialData[i][Right].rgbGreen)/5+0.5;
				BMPPartialSaveData[i][j].rgbRed =  (double) (BMPPartialData[i][j].rgbRed+BMPPartialData[Top][j].rgbRed+BMPPartialData[Down][j].rgbRed+BMPPartialData[i][Left].rgbRed+BMPPartialData[i][Right].rgbRed)/5+0.5;
			}
	}


	MPI_Gatherv((char*)BMPPartialSaveData[1], send_counts[rank], BMP_RAW, rank==0 ? (char*)BMPSaveData[0] : NULL, send_counts, displaces, BMP_RAW, 0, MPI_COMM_WORLD);

	if(rank == 0){
		if(saveBMP(outfileName))
			cout << "Save file successfully!!" << endl;
		else
			cout << "Save file fails!!" << endl;

		//得到結束時間，並印出執行時間
		cout << "time used = " << MPI_Wtime() - startwtime << endl;
	}

	// free memory allocated
	if(rank == 0)
		free(BMPSaveData);
	free(BMPPartialSaveData);
	free(BMPPartialData);
	MPI_Finalize();
	return 0;
}

/*********************************************************/
/* 讀取圖檔                                              */
/*********************************************************/
int readBMP(char *fileName)
{
	//建立輸入檔案物件  
	ifstream bmpFile( fileName, ios::in | ios::binary );

	//檔案無法開啟
	if ( !bmpFile ){
		cout << "It can't open file!!" << endl;
		return 0;
	}

	//讀取BMP圖檔的標頭資料
	bmpFile.read( ( char* ) &bmpHeader, sizeof( BMPHEADER ) );

	//判決是否為BMP圖檔
	if( bmpHeader.bfType != 0x4d42 ){
		cout << "This file is not .BMP!!" << endl ;
		return 0;
	}

	//讀取BMP的資訊
	bmpFile.read( ( char* ) &bmpInfo, sizeof( BMPINFO ) );

	//判斷位元深度是否為24 bits
	if ( bmpInfo.biBitCount != 24 ){
		cout << "The file is not 24 bits!!" << endl;
		return 0;
	}

	//修正圖片的寬度為4的倍數
	while( bmpInfo.biWidth % 4 != 0 )
		bmpInfo.biWidth++;

	//動態分配記憶體
	BMPSaveData = alloc_memory( bmpInfo.biHeight, bmpInfo.biWidth);

	//讀取像素資料
	//for(int i = 0; i < bmpInfo.biHeight; i++)
	//  bmpFile.read( (char* )BMPSaveData[i], bmpInfo.biWidth*sizeof(RGBTRIPLE));
	bmpFile.read( (char* )BMPSaveData[0], bmpInfo.biWidth*sizeof(RGBTRIPLE)*bmpInfo.biHeight);

	//關閉檔案
	bmpFile.close();

	return 1;

}
/*********************************************************/
/* 儲存圖檔                                              */
/*********************************************************/
int saveBMP( char *fileName)
{
	//判決是否為BMP圖檔
	if( bmpHeader.bfType != 0x4d42 ){
		cout << "This file is not .BMP!!" << endl ;
		return 0;
	}

	//建立輸出檔案物件
	ofstream newFile( fileName,  ios:: out | ios::binary );

	//檔案無法建立
	if ( !newFile ){
		cout << "The File can't create!!" << endl;
		return 0;
	}

	//寫入BMP圖檔的標頭資料
	newFile.write( ( char* )&bmpHeader, sizeof( BMPHEADER ) );

	//寫入BMP的資訊
	newFile.write( ( char* )&bmpInfo, sizeof( BMPINFO ) );

	//寫入像素資料
	//for( int i = 0; i < bmpInfo.biHeight; i++ )
	//        newFile.write( ( char* )BMPSaveData[i], bmpInfo.biWidth*sizeof(RGBTRIPLE) );
	newFile.write( ( char* )BMPSaveData[0], bmpInfo.biWidth*sizeof(RGBTRIPLE)*bmpInfo.biHeight );

	//寫入檔案
	newFile.close();

	return 1;

}


/*********************************************************/
/* 分配記憶體：回傳為Y*X的矩陣                           */
/*********************************************************/
RGBTRIPLE **alloc_memory(int Y, int X )
{        
	//建立長度為Y的指標陣列
	RGBTRIPLE **temp = new RGBTRIPLE *[ Y ];
	RGBTRIPLE *temp2 = new RGBTRIPLE [ Y * X ];
	memset( temp, 0, sizeof( RGBTRIPLE ) * Y);
	memset( temp2, 0, sizeof( RGBTRIPLE ) * Y * X );

	//對每個指標陣列裡的指標宣告一個長度為X的陣列 
	for( int i = 0; i < Y; i++){
		temp[ i ] = &temp2[i*X];
	}

	return temp;

}
/*********************************************************/
/* 交換二個指標                                          */
/*********************************************************/
void swap(RGBTRIPLE *a, RGBTRIPLE *b)
{
	RGBTRIPLE *temp;
	temp = a;
	a = b;
	b = temp;
}


