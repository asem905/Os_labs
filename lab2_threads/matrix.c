#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>


int **A, **B, **C;
int row_matrixA, colA_rowB_matrix, col_matrixB;


typedef struct {
    int row;
    int col;
} ThreadData;


void readingFromMtrx(char *flName, int ***db_ptr_to_mtrx, int *rows, int *cols) {
    FILE *file = fopen(flName, "r");
    if (!file) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    fscanf(file, "row=%d col=%d\n", rows, cols);
    *db_ptr_to_mtrx = malloc(*rows * sizeof(int *));
    for (int i = 0; i < *rows; i++) {
        (*db_ptr_to_mtrx)[i] = malloc(*cols * sizeof(int));
        for (int j = 0; j < *cols; j++) {
            fscanf(file, "%d", &(*db_ptr_to_mtrx)[i][j]);
        }
    }
    fclose(file);
}

void writeingInMatrix(char *flName, int **db_ptr_to_mtrx, int rows, int cols) {
    FILE *file = fopen(flName, "w");
    if (!file) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    fprintf(file, "row=%d col=%d\n", rows, cols);
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            fprintf(file, "%d ", db_ptr_to_mtrx[i][j]);
        }
        fprintf(file, "\n");
    }
    fclose(file);
}

void *multiplyPerMatrix(void *arg) {
    for (int i = 0; i < row_matrixA; i++) {
        for (int j = 0; j < col_matrixB; j++) {
            C[i][j] = 0;
            for (int k = 0; k < colA_rowB_matrix; k++) {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }
    pthread_exit(NULL);
}


void *multiplyPerRow(void *arg) {
    int row = *((int *)arg);
    free(arg);
    for (int j = 0; j < col_matrixB; j++) {
        C[row][j] = 0;
        for (int k = 0; k < colA_rowB_matrix; k++) {
            C[row][j] += A[row][k] * B[k][j];
        }
    }
    pthread_exit(NULL);
}


void *multiplyPerElement(void *arg) {
    ThreadData *thrdData = (ThreadData *)arg;
    int row = thrdData->row;
    int col = thrdData->col;
    free(thrdData);
    C[row][col] = 0;
    for (int k = 0; k < colA_rowB_matrix; k++) {
        C[row][col] += A[row][k] * B[k][col];
    }
    pthread_exit(NULL);
}


double measureTime(void (*func)(char *), char *flName, char *methodName) {
    struct timeval start, stop;
    gettimeofday(&start, NULL);

    func(flName);

    gettimeofday(&stop, NULL);
    double seconds = (stop.tv_sec - start.tv_sec) + (stop.tv_usec - start.tv_usec) / 1e6;
    printf("%s - Execution Time: %.6f seconds\n", methodName, seconds);
    return seconds;
}

void exec_method1(char *outfile) {
    pthread_t thread;
    pthread_create(&thread, NULL, multiplyPerMatrix, NULL);
    pthread_join(thread, NULL);
    writeingInMatrix(outfile, C, row_matrixA, col_matrixB);
}


void exec_method2(char *outfile) {
    pthread_t rwThrds[row_matrixA];
    for (int i = 0; i < row_matrixA; i++) {
        int *row = malloc(sizeof(int));
        *row = i;
        pthread_create(&rwThrds[i], NULL, multiplyPerRow, row);
    }
    for (int i = 0; i < row_matrixA; i++) {
        pthread_join(rwThrds[i], NULL);
    }
    writeingInMatrix(outfile, C, row_matrixA, col_matrixB);
}


void exec_method3(char *outfile) {
    pthread_t elementThreads[row_matrixA * col_matrixB];
    for (int i = 0; i < row_matrixA; i++) {
        for (int j = 0; j < col_matrixB; j++) {
            ThreadData *thrdData = malloc(sizeof(ThreadData));
            thrdData->row = i;
            thrdData->col = j;
            pthread_create(&elementThreads[i * col_matrixB + j], NULL, multiplyPerElement, thrdData);
        }
    }
    for (int i = 0; i < row_matrixA * col_matrixB; i++) {
        pthread_join(elementThreads[i], NULL);
    }
    writeingInMatrix(outfile, C, row_matrixA, col_matrixB);
}


int main(int argc, char *argv[]) {
    char *mat1Fil = (argc > 1) ? argv[1] : "a.txt";
    char *mat2Fil = (argc > 2) ? argv[2] : "b.txt";
    char *outputPrefix = (argc > 3) ? argv[3] : "c";


    readingFromMtrx(mat1Fil, &A, &row_matrixA, &colA_rowB_matrix);
    readingFromMtrx(mat2Fil, &B, &colA_rowB_matrix, &col_matrixB);

    C = malloc(row_matrixA * sizeof(int *));
    for (int i = 0; i < row_matrixA; i++) {
        C[i] = malloc(col_matrixB * sizeof(int));
    }

    char out1[100], out2[100], out3[100];
    sprintf(out1, "%s_per_matrix.txt", outputPrefix);
    sprintf(out2, "%s_per_row.txt", outputPrefix);
    sprintf(out3, "%s_per_element.txt", outputPrefix);


    measureTime(exec_method1, out1, "Method 1 (A thread per matrix)");
    measureTime(exec_method2, out2, "Method 2 (A thread per row)");
    measureTime(exec_method3, out3, "Method 3 (A thread per element)");


    for (int i = 0; i < row_matrixA; i++) {
        free(A[i]);
        free(C[i]);
    }
    for (int i = 0; i < colA_rowB_matrix; i++) {
        free(B[i]);
    }
    free(A);
    free(B);
    free(C);

    return 0;
}
