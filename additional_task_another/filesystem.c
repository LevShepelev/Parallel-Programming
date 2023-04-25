#include <stdio.h>
#include <string.h>
#include <mpi.h>

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);
    int my_rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    MPI_File common_file;
    
    if (MPI_File_open(MPI_COMM_WORLD, "Common_file.txt", MPI_MODE_CREATE | MPI_MODE_EXCL | MPI_MODE_RDWR, MPI_INFO_NULL, &common_file) != MPI_SUCCESS) {
        printf("The file already exists.\n");
        MPI_Finalize();
        return 0;
    }
    MPI_File_close(&common_file);

    if (my_rank == size - 1) {
        MPI_File_open(MPI_COMM_SELF, "Common_file.txt", MPI_MODE_RDWR, MPI_INFO_NULL, &common_file);
        MPI_File_write(common_file, "Hi, My rank is ", strlen("Hi, My rank is "), MPI_CHAR, MPI_STATUS_IGNORE);
        
        if (my_rank < 10) {
            char value[2];
            sprintf(value, "%d", my_rank);
            MPI_File_write(common_file, &value[0], 1, MPI_CHAR, MPI_STATUS_IGNORE);
        }
        else {
            char value[3];
            sprintf(value, "%d", my_rank);
            MPI_File_write(common_file, &value[0], 2, MPI_CHAR, MPI_STATUS_IGNORE);
        }

        MPI_File_write(common_file, "!\n", strlen("!\n"), MPI_CHAR, MPI_STATUS_IGNORE);
        MPI_File_close(&common_file);
        
        char ack = '+';
        MPI_Send(&ack, 1, MPI_CHAR, size - 2, 179, MPI_COMM_WORLD);
    }

    if ((my_rank < size - 1) & (my_rank > 0)) {
        char ack;
        MPI_Recv(&ack, 1, MPI_CHAR, my_rank + 1, 179, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        MPI_File_open(MPI_COMM_SELF, "Common_file.txt", MPI_MODE_RDWR | MPI_MODE_APPEND, MPI_INFO_NULL, &common_file);
        MPI_File_write(common_file, "Hello! My rank is ", strlen("Hello! My rank is "), MPI_CHAR, MPI_STATUS_IGNORE);
        
        if (my_rank < 10) {
            char value[2];
            sprintf(value, "%d", my_rank);
            MPI_File_write(common_file, &value[0], 1, MPI_CHAR, MPI_STATUS_IGNORE);
        }
        else {
            char value[3];
            sprintf(value, "%d", my_rank);
            MPI_File_write(common_file, &value[0], 2, MPI_CHAR, MPI_STATUS_IGNORE);
        }

        MPI_File_write(common_file, "!\n", strlen("!\n"), MPI_CHAR, MPI_STATUS_IGNORE);
        MPI_File_close(&common_file);

        MPI_Send(&ack, 1, MPI_CHAR, my_rank - 1, 179, MPI_COMM_WORLD);
    }

    if (my_rank == 0) {
        char ack;
        MPI_Recv(&ack, 1, MPI_CHAR, 1, 179, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        MPI_File_open(MPI_COMM_SELF, "Common_file.txt", MPI_MODE_RDWR | MPI_MODE_APPEND, MPI_INFO_NULL, &common_file);
        MPI_File_write(common_file, "Hello! My rank is ", strlen("Hello! My rank is "), MPI_CHAR, MPI_STATUS_IGNORE);
        char value[2];
        sprintf(value, "%d", my_rank);
        MPI_File_write(common_file, &value[0], 1, MPI_CHAR, MPI_STATUS_IGNORE);
        MPI_File_write(common_file, "!", strlen("!"), MPI_CHAR, MPI_STATUS_IGNORE);
        MPI_File_close(&common_file);
    }
    MPI_Finalize();
    return 0;
}