#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]){
        int commsize, my_rank;
        MPI_Init(&argc,&argv);
        MPI_Comm_size(MPI_COMM_WORLD, &commsize);
        MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

        if (!my_rank) {

                //int n_Cores = atoi(argv[1]) - 1;
                //int* numbers = (int*)calloc(n_Cores, sizeof(int));
                int Sum = 0;
                MPI_Send(&Sum, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
                MPI_Recv(&Sum, 1, MPI_INT, commsize - 1, 0, MPI_COMM_WORLD, NULL);
                printf("%d\n", Sum);
        }

        else {
                int Sum = 0;
                MPI_Recv(&Sum, 1, MPI_INT, my_rank - 1, 0, MPI_COMM_WORLD, NULL);
                Sum +=1;
                MPI_Send(&Sum, 1, MPI_INT, (my_rank + 1)%commsize, 0, MPI_COMM_WORLD);
                printf("My rank is %d and my summ is %d\n", my_rank, Sum);
        }

        MPI_Finalize();
}

