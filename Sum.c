#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]){
        int N = atoi(argv[1]);
        int commsize, my_rank;
        MPI_Init(&argc,&argv);
        MPI_Comm_size(MPI_COMM_WORLD, &commsize);
        MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
        MPI_Status* status;
        if (!my_rank) {

                //int n_Cores = atoi(argv[1]) - 1;
                //int* numbers = (int*)calloc(n_Cores, sizeof(int));
                double Sum = 0;
                for (int i = 0; i < commsize-1; ++i) {

                        double recv  = 0;
                        MPI_Recv(&recv, 1, MPI_DOUBLE, MPI_ANY_SOURCE, my_rank, MPI_COMM_WORLD, status);

                        Sum += recv;
                }
                printf("%f\n", Sum);
                //free(numbers);
        }
        else
                {
                double Sum = 0.;
                for (int i = (my_rank - 1) * N / (commsize - 1); i < (my_rank) * N / (commsize - 1); ++i)
                        Sum +=  1./(i + 1);
                printf("My rank is %d and my summ is %f\n", my_rank, Sum);
                MPI_Send(&Sum, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);

        }
        MPI_Finalize();
}

