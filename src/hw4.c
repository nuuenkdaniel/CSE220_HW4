#include <asm-generic/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUFFER_SIZE 1024
#define PORT 8080

int main() {
  int listen_fd, conn_fd;
  struct sockaddr_in address;
  int opt = 1;
  int addrlen = sizeof(address);
  char buffer[BUFFER_SIZE] = {0};

  // Creating socket
  if((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    perror("Could not create socket");
    exit(EXIT_FAILURE);
  }

  // Setting socket options
  if(setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
    perror("Failed to set reuse address option for socket");
    exit(EXIT_FAILURE);
  }
  if(setsockopt(listen_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt))) {
    perror("Failed to set reuse port option for socket");
    exit(EXIT_FAILURE);
  }

  // Binding socket to port
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);
  if(bind(listen_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    perror("Failed to bind socket to port");
    exit(EXIT_FAILURE);
  }

  // Listening for connections
  if(listen(listen_fd, 2) < 0) {
     perror("Failed to listen for connections");
     exit(EXIT_FAILURE);
  }
  printf("[Server] listening on port: %d\n", PORT);
  
  // Accept incoming connetions
  if((conn_fd = accept(listen_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
    perror("Failed to accept connection"); 
    exit(EXIT_FAILURE);
  }

  while(1) {
    // do something
  }

  printf("[Server] Shutting down.\n");
  close(conn_fd);
  close(listen_fd);
  return EXIT_SUCCESS;
}
