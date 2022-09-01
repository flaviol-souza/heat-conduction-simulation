#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <png.h>
#include <gd.h>

#define ITERATIONS 150000
#define INIT_TEMP_HOT 100
#define INIT_TEMP_NEUTRAL 0
#define COLUMN 300
#define ROW    300
#define CHUNKSIZE 5
#define SIZE_IMAGE 10

time_t t1, t2;

struct Whole_matrix {
	double calculated[ROW][COLUMN];
	double updated[ROW][COLUMN];
};

enum TYPE_IMAGE {IMAGE_BEGIN, IMAGE_END};

void outputMatrixToPNG(double matrix[][COLUMN],enum TYPE_IMAGE type) {
	int maxvalue =100;
	int i, j, tot = 0;
	FILE *pngout;

	gdImagePtr im = gdImageCreate(COLUMN, ROW);

	gdImageColorAllocate(im, 255, 255, 255);

	for (i = 0; i < COLUMN; i++) {
		for (j =0; j < ROW; j++) {
			int valor = 100 - (int)(matrix[i][j]);
			int color = gdImageColorExact(im, valor, valor, valor) ;
			if (color == -1) {
				color = gdImageColorAllocate(im, valor, valor, valor);
			}
			gdImageSetPixel(im, j, i, color);
		}
	}

	if (IMAGE_BEGIN == type)
		pngout = fopen("imageIcParallel_Begin.png", "wb");
	else if(IMAGE_END == type)
		pngout = fopen("imageIcParallel_End.png", "wb");

	gdImagePng(im, pngout);

	fclose(pngout);

	gdImageDestroy(im);

	return;
}

void mountsEmptyArray(double array[][COLUMN]) {
	//dividir em threads a criancao da matriz
	int i, j;
	/* The initial temperature of the first row for the first 
	 column takes the value of 'INIT_TEMP_HOT' */
	#pragma omp parallel for shared(array) private(j) schedule(static,CHUNKSIZE)
		for (j = 0; j < COLUMN; j++)
			array[0][j] = INIT_TEMP_HOT;

	#pragma omp parallel for shared(array) private(i,j) schedule(static,CHUNKSIZE)
		for (i = 1; i < ROW; i++) {
			for (j = 0; j < COLUMN; j++)
				array[i][j] = INIT_TEMP_NEUTRAL;
		}
}

void printArray(double array[][COLUMN]) {
	int i, j;
	printf("\n \n");
	for (i = 0; i < ROW; i++) {
		for (j = 0; j < COLUMN; j++)
			printf("| %f ", array[i][j]);
		printf("\n");
	}
}

/* The parameter 'i' represented the horizontal coordinate (row) 
 and the parameter 'j' the vertical coordinate (column) */
double calculatesNewTemperature(double array[][COLUMN], int i, int j) {
	int computes_dividers = 1;
	double accumulates_current_temp = array[i][j];

	// computes values of cells available in row
	if (i == 0) {
		accumulates_current_temp += array[i+1][j];
		computes_dividers += 1;
	} else if (i == ROW-1) {
		accumulates_current_temp += array[i-1][j];
		computes_dividers += 1;
	} else if (i > 0 && i < ROW-1) {
		accumulates_current_temp += array[i-1][j];
		accumulates_current_temp += array[i+1][j];
		computes_dividers += 2;
	}

	// computes values of cells available in column     
	if (j == 0) {
		accumulates_current_temp += array[i][j+1];
		computes_dividers += 1;
	} else if (j == COLUMN-1) {
		accumulates_current_temp += array[i][j-1];
		computes_dividers += 1;
	} else if (j > 0 && j < COLUMN-1) {
		accumulates_current_temp += array[i][j-1];
		accumulates_current_temp += array[i][j+1];
		computes_dividers += 2;
	}

	return (accumulates_current_temp / computes_dividers);
}

void calculateHeatTransfer(double updated[][COLUMN],double calculated[][COLUMN]) {
	int i, j;
	#pragma omp parallel for shared(updated, calculated) private (i,j) schedule(static,CHUNKSIZE)
		for (i = 1; i < ROW; i++) {
	//#pragma omp parallel for shared (updated, calculated, i) private(j) schedule(dynamic) 
		for (j = 0; j < COLUMN; j++)
			updated[i][j] = calculatesNewTemperature(calculated, i, j);
	}
}

int main(int argc, char *argv[]) {
	//Thread master
	(void) time(&t1);

	struct Whole_matrix matrix;
	#pragma omp master
	{
		mountsEmptyArray(matrix.calculated);
		mountsEmptyArray(matrix.updated);

		if (ITERATIONS%2 == 0){
			outputMatrixToPNG(matrix.calculated,IMAGE_BEGIN);
//			printArray(matrix.calculated);
		}
		else{
			outputMatrixToPNG(matrix.updated,IMAGE_BEGIN);
//			printArray(matrix.updated);
		}
		
		int cont = 0;
		while (cont <= ITERATIONS) {
			if (cont%2 == 0)
				calculateHeatTransfer(matrix.updated, matrix.calculated);
			else
				calculateHeatTransfer(matrix.calculated, matrix.updated);
			cont++;
		}

		(void) time(&t2);
		printf("\nO tempo de processamento foi de %f segundos.\n", (double)t2-t1);

		if (ITERATIONS%2 == 0){
			outputMatrixToPNG(matrix.calculated,IMAGE_END);
//			printArray(matrix.calculated);
		}
		else{
			outputMatrixToPNG(matrix.updated,IMAGE_END);
//			printArray(matrix.updated);
		}
	}

	return 0;
}