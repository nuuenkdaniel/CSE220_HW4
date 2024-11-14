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

void copy_shape(int *src_shape, int *dest_shape, int r, int c_src, int c_dest) {
  for(int i = 0; i < r; i++) {
    for(int j = 0; j < c_src; j++) {
      if(src_shape[i*c_src+j] == 1) dest_shape[i*c_dest+j] = 1;
    }
  }
}

// Rotates the given shape by the give number of times clockwise
int *rotate_shape(int *shape, int r, int c, int rotations){ 
  int temp_shape[16] = {0};
  copy_shape(shape, (int*)temp_shape, r, c, 4);
  int *new_shape = calloc(r*c, sizeof(r*c));
  int temp;
  int curr_left;
  int curr_right;
  int largest = (r > c)? r : c;

  for(int i = 0; i < rotations; i++) {
    // Transposing the matrix
    for(int j = 0; j < largest; j++) {
      for(int k = j+1; k < largest; k++) {
        temp = temp_shape[j*4+k];
        temp_shape[j*4+k] = temp_shape[k*4+j];
        temp_shape[k*4+j] = temp;
      }
    }

    // Swap row and column indexes
    temp = r;
    r = c;
    c = temp;
    // Flipping the columns of the matrix
    for(int j = 0; j < largest; j++) {
      curr_left = 0;
      curr_right = largest-1;
      while(curr_left < curr_right) {
        temp = temp_shape[j*4+curr_left];
        temp_shape[j*4+curr_left] = temp_shape[j*4+curr_right];
        temp_shape[j*4+curr_right] = temp;
        curr_left++;
        curr_right--;
      }
    }
  }

  // Fix shape back to top left
  int i,j;
  i = 0;
  temp = 0;
  while(i < 4) {
    j = 0;
    while(j < 4) {
      if(temp_shape[i*4+j]) break;
      j++;
    }
    if(j < 4) break;
    temp++;
    i++;
  }

  j = 0;
  int temp2 = 0;
  while(j < 4) {
    i = 0;
    while(i < 4) {
      if(temp_shape[i*4+j]) break;
      i++;
    }
    if(i < 4) break;
    temp2++;
    j++;
  }

  for(int i = temp; i < 4; i++) {
    for(int j = temp2; j < 4; j++) {
      temp_shape[(i-temp)*4+(j-temp2)] = temp_shape[i*4+j];
      if(temp != 0 || temp2 != 0) temp_shape[i*4+j] = 0;
    }
  }
  copy_shape((int*)temp_shape, new_shape, r, 4, c);
  return new_shape;
}

void read_fd(int conn_fd, char *buffer) {
  memset(buffer, 0, BUFFER_SIZE);
  int nbytes = read(conn_fd, buffer, BUFFER_SIZE);
  if(nbytes <= 0) {
    perror("Failed to read from client");
    exit(EXIT_FAILURE);
  }
}

void begin_game(int *board1, int *board2, int conn_fd1, int conn_fd2, char *buffer) {
  int done = 0;
  char extra[1024];
  char packet_type;
  while(!done) {
    read_fd(conn_fd1, buffer);
    int args[3];
    if(sscanf(buffer, "%c %d %d %s", &packet_type, &args[0], &args[1], extra) == 3 && packet_type == 'B' && args[0] > 0 && args[1] > 0) {
      done = 1; 
      board1 = calloc(args[0]*args[1], sizeof(int));
      board2 = calloc(args[0]*args[1], sizeof(int));
      send(conn_fd1, "A", 1, 0);
    }
    else if(packet_type != 'B') send(conn_fd1, "100", 3, 0);
    else send(conn_fd1, "200", 3, 0);
  }
  done = 0;
  while(!done) {
    read_fd(conn_fd2, buffer);
    if(sscanf(buffer, "%c %s", &packet_type, extra) == 1 && packet_type == 'B') {
      done = 1;
      send(conn_fd2, "A", 1, 0);
    }
    else if(packet_type != 'B') send(conn_fd2, "100", 3, 0);
    else send(conn_fd2, "200", 3, 0);
  }
}

void initialize_game(int *board1, int conn_fd, char *buffer) {
  int done = 0;
  char packet_type;
  char extra[1024];
  while(!done) {
    read_fd(conn_fd, buffer);
    int args[5][4];
    char *buffer_ptr = buffer;
    if(sscanf(buffer_ptr, "%c", &packet_type) != 1 || packet_type != 'I') send(conn_fd, "101", 3, 0);
    buffer_ptr = strchr(buffer_ptr, ' ');
    int i = 0;
    while(i < 5) {
      if(sscanf(buffer_ptr, "%d %d %d %d", &args[i][0], &args[i][1], &args[i][2], &args[i][3]) != 4) send(conn_fd, "201", 3, 0);
    }
  }
}

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

  int *board1, *board2 = NULL;
  char player1_history[1024], board2_history[1024];
  player1_history[0] = '\0';
  board2_history[0] = '\0';

  begin_game(board1, board2, conn_fd1, conn_fd2, buffer);
  // initialize_game(board1, conn_fd1, buffer);

  printf("[Server] Shutting down.\n");
  free(board1);
  free(board2);
  close(conn_fd1);
  close(listen_fd1);
  close(conn_fd2);
  close(listen_fd2);
  return EXIT_SUCCESS;
}
