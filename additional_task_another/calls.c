#include <stdio.h>
#include <mpi.h>

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);
    int size, my_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    int variable;
    MPI_Win our_window;
    MPI_Win_create(&variable, sizeof(int), sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &our_window);
    MPI_Win_fence(0, our_window);

    if (my_rank == 0) {
        int initial_value = 0;
        printf("My rank: %d, initial value: %d\n", my_rank, initial_value);
        MPI_Put(&initial_value, 1, MPI_INT, 1, 0, 1, MPI_INT, our_window);
    }
    MPI_Win_fence(0, our_window);

    for (int i = 1; i < size - 1; i++) {
        if (my_rank == i) {
            variable += 10;
            printf("My rank: %d, intermediate value: %d\n", my_rank, variable);
            MPI_Put(&variable, 1, MPI_INT, i + 1, 0, 1, MPI_INT, our_window);
        }
        MPI_Win_fence(0, our_window);
    }

    if (my_rank == size - 1) {
        variable += 10;
        printf("My rank: %d, value: %d\n", my_rank, variable);
        MPI_Put(&variable, 1, MPI_INT, 0, 0, 1, MPI_INT, our_window);
    }
    MPI_Win_fence(0, our_window);

    if (my_rank == 0) {
        variable += 10;
        printf("final value: %d\n", variable);
    }
    MPI_Win_free(&our_window);
    MPI_Finalize();
    return 0;
}