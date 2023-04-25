#include <stdio.h>
#include <string.h>
#include <mpi.h>

int main(int argc, char *argv[]) {
   MPI_Init(&argc, &argv);
   char portname[MPI_MAX_PORT_NAME];
   MPI_Lookup_name("https://mipt.ru/", MPI_INFO_NULL, portname);

   MPI_Comm server;
   MPI_Comm_connect(portname, MPI_INFO_NULL, 0, MPI_COMM_SELF, &server);
   printf("A client connected to the server with the port: %s\n", portname);

   char received_message[100] = {'\0'};
   MPI_Recv(&received_message, 100, MPI_CHAR, 0, 357, server, MPI_STATUS_IGNORE);
   printf("I'm a client, I received a message: << %s >>\n", received_message);

   MPI_Send("Hello, server!", strlen("Hello, server!"),
            MPI_CHAR, 0, 768, server);
   
   MPI_Finalize();
   return 0;
}