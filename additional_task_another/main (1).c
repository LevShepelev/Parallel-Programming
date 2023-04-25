#include "mpi.h"

int main(int argc, char *argv[]) {
   MPI_Init(&argc, &argv);
   MPI_Comm inter_comm;
   MPI_Comm_spawn("server.out", MPI_ARGV_NULL, 1,
                  MPI_INFO_NULL, 0, MPI_COMM_SELF,
                  &inter_comm, MPI_ERRCODES_IGNORE);
   MPI_Comm_spawn("client.out", MPI_ARGV_NULL, 1,
                  MPI_INFO_NULL, 0, MPI_COMM_SELF,
                  &inter_comm, MPI_ERRCODES_IGNORE);
   MPI_Finalize();
   return 0;
}