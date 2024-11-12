#include <asm-generic/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUFFER_SIZE 1024
#define PORT1 2201
#define PORT2 2202

// Sets up the socket for connection
void socket_setup(int *listen_fd, struct sockaddr_in *address, int *opt, int *addrlen, short port) {
  // Creating socket
  if((*listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    perror("Could not create socket");
    exit(EXIT_FAILURE);
  }

  // Setting socket options
  if(setsockopt(*listen_fd, SOL_SOCKET, SO_REUSEADDR, opt, sizeof(*opt))) {
    perror("Failed to set reuse address option for socket");
    exit(EXIT_FAILURE);
  }
  if(setsockopt(*listen_fd, SOL_SOCKET, SO_REUSEPORT, opt, sizeof(*opt))) {
    perror("Failed to set reuse port option for socket");
    exit(EXIT_FAILURE);
  }

  // Binding socket to port
  address->sin_family = AF_INET;
  address->sin_addr.s_addr = INADDR_ANY;
  address->sin_port = htons((port == 0)? PORT1 : PORT2);
  if(bind(*listen_fd, (struct sockaddr *)address, sizeof(*address)) < 0) {
    perror("Failed to bind socket to port");
    exit(EXIT_FAILURE);
  }

  // Listening for connections
  if(listen(*listen_fd, 2) < 0) {
    perror("Failed to listen for connections");
    exit(EXIT_FAILURE);
  }
}

// Accept incoming connections on specified socket
void await_conn(int *listen_fd, int *conn_fd, struct sockaddr_in *address, int *addrlen) {
  if((*conn_fd = accept(*listen_fd, (struct sockaddr *)address, (socklen_t *)addrlen)) < 0) {
    perror("Failed to accept connection");
    exit(EXIT_FAILURE);
  }
}

// Rotates the given shape by the give number of times clockwise
int *rotate_shape(int *shape, int r, int c, int rotations){ 
  int temp_shape[6] = {0};
  memcpy(temp_shape, shape, r*c*sizeof(int));
  int *new_shape = calloc(r*c, sizeof(r*c));
  int temp;
  int curr_left;
  int curr_right;

  for(int i = 0; i < 2; i++) {
    for(int j = 0; j < 3; j++) {
      printf("%d ", temp_shape[i*4+j]);
    }
    printf("\n");
  }


  for(int i = 0; i < rotations; i++) {
    // Transposing the matrix
    for(int j = 0; j < r; j++) {
      for(int k = 0; k < c; k++) {
        temp = temp_shape[j*c+k];
        temp_shape[j*c+k] = temp_shape[k*c+j];
        temp_shape[k*c+j] = temp;
      }
    }
    temp = r;
    r = c;
    c = temp;
    // Flipping the columns of the matrix
    for(int j = 0; j < r; j++) {
      curr_left = 0;
      curr_right = c-1;
      while(curr_left < curr_right) {
        temp = temp_shape[j*c+curr_left];
        temp_shape[j*c+curr_left] = temp_shape[j*c+curr_right];
        temp_shape[j*c+curr_right] = temp;
        curr_left++;
        curr_right--;
      }
    }
  }
  memcpy(new_shape, temp_shape, r*c*sizeof(int));
  return new_shape;
}

void handle_request(){};

int main() {
  int listen_fd1, listen_fd2;
  int conn_fd1, conn_fd2;
  struct sockaddr_in address1, address2;
  int opt = 1;
  int addrlen = sizeof(address1);
  char buffer[BUFFER_SIZE] = {0};
  
  // Socket setup
  socket_setup(&listen_fd1, &address1, &opt, &addrlen, 0);
  socket_setup(&listen_fd2, &address2, &opt, &addrlen, 1);
  printf("[Server] listening on port %d and %d\n", PORT1, PORT2);

  // Accept Connections
  await_conn(&listen_fd1, &conn_fd1, &address1, &addrlen);
  await_conn(&listen_fd2, &conn_fd2, &address2, &addrlen);
  
  // Creating the shapes
  int shape1[2][2] = { 1,1,1,1 };
  int shape2[4][1] = { 1,1,1,1 };
  int shape3[2][3] = { 0,1,1,1,1,0 };
  int shape4[3][2] = { 1,0,1,0,1,1 };
  int shape5[2][3] = { 1,1,0,0,1,1 };
  int shape6[3][2] = { 0,1,0,1,1,1 };
  int shape7[2][3] = { 1,1,1,0,1,0 };

  printf("Got here\n");
  int *test_shape = rotate_shape((int*)shape3, 2, 3, 1);

  for(int i = 0; i < 3; i++) {
    for(int j = 0; j < 2; j++) {
      printf("%d \n", test_shape[i*2+j]);
    }
  }

  printf("Got here 2\n");

  /*
  int nbytes;
  while(1) {
    memset(buffer, 0, BUFFER_SIZE);
    nbytes = read(conn_fd1, buffer, BUFFER_SIZE);
    if(nbytes <= 0) {
      perror("Failed to read from client");
      exit(EXIT_FAILURE);
    }
    printf("Data recieved from client1: %s\n", buffer);
    memset(buffer, 1, BUFFER_SIZE);
    buffer[strlen(buffer)-1] = '\0';
    send(conn_fd1, "Hello", strlen("Hello"), 0);

    memset(buffer, 0, BUFFER_SIZE);
    nbytes = read(conn_fd1, buffer, BUFFER_SIZE);
    if(nbytes <= 0) {
      perror("Failed to read from client");
      exit(EXIT_FAILURE);
    }
    printf("Data recieved from client2: %s\n", buffer);
  }

  printf("[Server] Shutting down.\n");
  close(conn_fd1);
  close(listen_fd1);
  close(conn_fd2);
  close(listen_fd2);
  */
  return EXIT_SUCCESS;
}
