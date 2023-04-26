#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <mpi.h>

#ifndef M_PI
        #define M_PI 3.14159265358979
#endif

void* my_calloc(int n_elem, int size_of_elem) {
        void* ptr = calloc(n_elem, size_of_elem);
        if (ptr != NULL)
                return ptr;
        printf("calloc_error\n");
        return (void*)-1;
}

int main(int argc, char* argv[]) {
        MPI_Init(&argc, &argv);
        int my_rank, commsize;
        MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
        MPI_Comm_size(MPI_COMM_WORLD, &commsize);

        // Start counting time inside process zero
        double startTime;
        if (my_rank == 0)
                startTime = MPI_Wtime();

        // Initialize bounds of the task
        double T = 1.0, X = 1.0;
        double tau = 1e-4, h = 1e-2;

        // Total number of steps
        int T_steps = (int) T/tau;
        int X_steps = (int) X/h;

        // Initialize boundary conditions of the task and its parameter
        double a = 1.0;
        double* timeBoundary = (double*) my_calloc(T_steps, sizeof(double));
        double* distBoundary = (double*) my_calloc(X_steps, sizeof(double));
        int i;
        for (i = 0; i < T_steps; i++)
                timeBoundary[i] = -10.0 * sin(2. * M_PI * tau * i);
        for (i = 0; i < X_steps; i++)
                distBoundary[i] = i * h;
        
        // Initialize Courant number
        double courant = a * tau / h;

        // Allocate a part of the segment to each process
        int segmentSize = (int) X/(h * commsize);
        int startOfSegment = my_rank * segmentSize;
        printf("Start: %d.\tLength: %d.\n", startOfSegment, segmentSize);

        // Allocate memory for solution
        double** u = (double**) my_calloc(T_steps, sizeof(double*));
        for (i = 0; i < T_steps; i++)
                u[i] = (double*) my_calloc(segmentSize, sizeof(double));

        // First time layer
        for (i = 0; i < segmentSize; i++)
                u[0][i] = distBoundary[i + my_rank * segmentSize];

        // Second time layer (using left corner scheme)
        for (i = 1; i < segmentSize; i++)
                u[1][i] = courant * u[0][i-1] + (1. - courant) * u[0][i];
        if (my_rank != commsize - 1)
                MPI_Send(&u[0][segmentSize-1], 1, MPI_DOUBLE, my_rank + 1, 0, MPI_COMM_WORLD);
        if (my_rank != 0) {
                double tmp;
                MPI_Recv(&tmp, 1, MPI_DOUBLE, my_rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                u[1][0] = courant * tmp + (1. - courant) * u[0][0];
        }
        if (my_rank == 0)
                u[1][0] = timeBoundary[1];
        
        // Rest time layers (using cross scheme)
        int j; double tmp_behind, tmp_front;
        for (i = 2; i < T_steps; i++){
                for (j = 1; j < segmentSize - 1; j++)
                        u[i][j] = u[i - 2][j] + courant * (u[i - 1][j-1] - u[i - 1][j+1]);
                if (my_rank != commsize - 1)
                        MPI_Send(&u[i - 1][segmentSize-1], 1, MPI_DOUBLE, my_rank + 1, 0, MPI_COMM_WORLD);
                if (my_rank != 0)
                        MPI_Recv(&tmp_behind, 1, MPI_DOUBLE, my_rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                if (my_rank != 0)
                        MPI_Send(&u[i - 1][0], 1, MPI_DOUBLE, my_rank - 1, 0, MPI_COMM_WORLD);
                if (my_rank != commsize - 1)
                        MPI_Recv(&tmp_front, 1, MPI_DOUBLE, my_rank + 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                if (my_rank != commsize - 1)
                        u[i][segmentSize-1] = u[i - 2][segmentSize-1] + courant * (u[i - 1][segmentSize-2] - tmp_front);
                if (my_rank != 0)
                        u[i][0] = u[i - 2][0] + courant * (tmp_behind - u[i - 1][1]);
                if (my_rank == 0)
                        u[i][0] = timeBoundary[i];
                if (my_rank == commsize - 1)
                        u[i][segmentSize-1] = courant * u[i - 1][segmentSize-2] + (1. - courant) * u[i - 1][segmentSize-1];
        }

        // Count time
        double endTime;
        if (my_rank == 0){
                endTime = MPI_Wtime();
                printf("Time: %lg.\n", endTime - startTime);
        }
        if (my_rank != 0)
                for (int i = 0; i < T_steps; i++)
                        MPI_Send(u[i], segmentSize, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
        if (my_rank == 0) {
                printf("want to calloc %d\n", X_steps * T_steps * sizeof(double) / 1024 / 1024);
                double* solution = my_calloc (X_steps * T_steps, sizeof(double));
                        
                
                for (int i = 0; i < T_steps; i++){
                        //printf("recv\n");
                        memcpy(&(solution[i * X_steps]), u[i], segmentSize);
                        for (int j = 1; j < commsize; j++)
                                MPI_Recv(&(solution[i * X_steps + j * segmentSize]), segmentSize, MPI_DOUBLE, j, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                }       
                for (int j = 0; j < T_steps; j++) {
                        for (int i = 0; i < X_steps; i++)
                                printf ("%10lf ", solution[j*X_steps + i]);
                        printf("\n");
                }
                
                free (solution);
        }


        // Let's estimate accuracy of our solution
        double* analytics = (double*) calloc(segmentSize, sizeof(double));
        double metrics = 0;
        for (i = 0; i < segmentSize; i++){
                analytics[i] = sin(2 * M_PI * ((double) (i + my_rank * segmentSize) * (double)h - ((double) T_steps - 1.) * (double)tau));
                metrics += pow(analytics[i] - u[T_steps - 1][i], 2);
        }
        printf("My rank: %d.\tError: %g.\n", my_rank, sqrt(metrics));

        // Free memory
        free(timeBoundary);
        free(distBoundary);
        for (i = 0; i < T_steps; i++)
                free(u[i]);
        free(u);

        MPI_Finalize();
        return 0;
}