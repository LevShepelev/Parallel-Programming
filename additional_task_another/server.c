#include <stdio.h>
#include <string.h>
#include "mpi.h"

int main(int argc, char *argv[]) {  
   MPI_Init(&argc, &argv);

   char port_name[MPI_MAX_PORT_NAME];
   MPI_Open_port(MPI_INFO_NULL, port_name);
   printf("A server opened a port: %s\n", port_name);

   MPI_Comm client;
   MPI_Publish_name("https://mipt.ru/", MPI_INFO_NULL, port_name);
   MPI_Comm_accept(port_name, MPI_INFO_NULL, 0, MPI_COMM_WORLD, &client);

   MPI_Send("Welcome to the server!", strlen("Welcome to the server!"),
            MPI_CHAR, 0, 357, client);

   char received_message[100] = {'\0'};
   MPI_Recv(&received_message, 100, MPI_CHAR, 0, 768, client, MPI_STATUS_IGNORE);
   printf("I'm a server, I received a message: << %s >>\n", received_message);
   
   MPI_Unpublish_name("https://mipt.ru/", MPI_INFO_NULL, port_name);
   MPI_Close_port(port_name);
   MPI_Finalize();
   return 0;
}