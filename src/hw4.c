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

// BattleShip Board
typedef struct Board {
  int height;
  int width;
  int *board;
} Board;

// Creating the shapes
int shape1[2][2] = { 1,1,1,1 };
int shape2[4][1] = { 1,1,1,1 };
int shape3[2][3] = { 0,1,1,1,1,0 };
int shape4[3][2] = { 1,0,1,0,1,1 };
int shape5[2][3] = { 1,1,0,0,1,1 };
int shape6[3][2] = { 0,1,0,1,1,1 };
int shape7[2][3] = { 1,1,1,0,1,0 };



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

// Copies shape from src to dest
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

// Reads the given file descriptor and puts into buffer
void read_fd(int conn_fd, char *buffer) {
  memset(buffer, 0, BUFFER_SIZE);
  int nbytes = read(conn_fd, buffer, BUFFER_SIZE);
  if(nbytes <= 0) {
    perror("Failed to read from client");
    exit(EXIT_FAILURE);
  }
}

// Handles forfeiting
void handle_forfeit(int conn_fd1, int conn_fd2, int forfeiter, char *buffer) {
  if(forfeiter == 1) {
    read_fd(conn_fd2, buffer);
    send(conn_fd1, "H 0", 3, 0);
    send(conn_fd2, "H 1", 3, 0);
  }
  else {
    send(conn_fd1, "H 1", 3, 0);
    send(conn_fd2, "H 0", 3, 0);
  }
}

// Handles the 'B' packets
int begin_game(Board *board1, Board *board2, int conn_fd1, int conn_fd2, char *buffer) {
  int done = 0;
  char extra[1024];
  char packet_type;
  while(!done) {
    read_fd(conn_fd1, buffer);
    int args[3];
    if(sscanf(buffer, "%c %d %d %s", &packet_type, &args[0], &args[1], extra) == 3 && packet_type == 'B' && args[0] > 0 && args[1] > 0) {
      done = 1; 
      board1->width = args[0];
      board2->width = args[0];
      board1->height = args[1];
      board2->height = args[1];
      board1->board = calloc(args[0]*args[1], sizeof(int));
      board2->board = calloc(args[0]*args[1], sizeof(int));
      send(conn_fd1, "A", 1, 0);
    }
    else if(packet_type == 'F') return 1;
    else if(packet_type != 'B') send(conn_fd1, "E 100", 5, 0);
    else send(conn_fd1, "E 200", 5, 0);
  }
  done = 0;
  while(!done) {
    read_fd(conn_fd2, buffer);
    if(sscanf(buffer, "%c %s", &packet_type, extra) == 1 && packet_type == 'B') {
      done = 1;
      send(conn_fd2, "A", 1, 0);
    }
    else if(packet_type != 'B') send(conn_fd2, "E 100", 5, 0);
    else send(conn_fd2, "E 200", 5, 0);
  }
  return 0;
}

// Initializes the board with the given piece information
int initialize_board(Board *board, int *piece_info, int conn_fd) {
  typedef struct {
    int *shape;
    int r;
    int c;
    int y_offset;
  } Shape;

  Shape shape[5];
  // Check for out of range
  for(int i = 0; i < 5; i++) {
    if(piece_info[i*4+0] > 7 || piece_info[i*4+0] < 0) {
      send(conn_fd, "E 300", 5, 0);
      return 0;
    }
  }

  // Check for rotation out of range
  for(int i = 0; i < 5; i++) {
    if(piece_info[i*4+1] > 4 || piece_info[i*4+1] < 1) {
      send(conn_fd, "E 301", 5, 0);
      return 0;
    }
  }

  // Check for doesn't fit
  for(int i = 0; i < 5; i++) {
    // Match the shape
    switch (piece_info[i*4+0]) {
      case 1:
        shape[i].shape = (int*)shape1;
        shape[i].r = 2;
        shape[i].c = 2;
        break;
      case 2:
        shape[i].shape = (int*)shape2;
        shape[i].r = 4;
        shape[i].c = 1;
        break;
      case 3:
        shape[i].shape = (int*)shape3;
        shape[i].r = 2;
        shape[i].c = 3;
        break;
      case 4:
        shape[i].shape = (int*)shape4;
        shape[i].r = 3;
        shape[i].c = 2;
        break;
      case 5:
        shape[i].shape = (int*)shape5;
        shape[i].r = 2;
        shape[i].c = 3;
        break;
      case 6:
        shape[i].shape = (int*)shape6;
        shape[i].r = 3;
        shape[i].c = 2;
        break;
      case 7:
        shape[i].shape = (int*)shape7;
        shape[i].r = 2;
        shape[i].c = 3;
        break;
    }

    // Rotate Shape
    shape[i].shape = rotate_shape(shape[i].shape, shape[i].r, shape[i].c, piece_info[i*4+1]-1);
    
    if(piece_info[i*4+1]%2 == 0) {
      int temp = shape[i].r;
      shape[i].r = shape[i].c;
      shape[i].c = temp;
    }
    // Figure out the y offset
    shape[i].y_offset = 0;
    while(shape[i].shape[shape[i].y_offset*4+0] != 1 && shape[i].y_offset < shape[i].r) shape[i].y_offset++;

    // Check if piece in range of the board
    if((piece_info[i*4+2]+shape[i].c-1) > board->width || piece_info[i*4+2] < 0 || piece_info[i*4+3]-shape[i].y_offset < 0 || (piece_info[i*4+3]-shape[i].y_offset+shape[i].r-1) > board->height) {
      send(conn_fd, "E 302", 5, 0);
      for(int j = 0; j < 5; j++) free(shape[j].shape);
      return 0;
    }
  }
  // Placing the piece while checking for collisions
  for(int i = 0; i < 5; i++) {
    int k = 0;
    int l = 0;
    while(k < shape[i].r) {
      l = 0;
      while(l < shape[i].c) {
        if(shape[i].shape[k*shape[i].c+l] == 1) {
          if(board->board[(piece_info[i*4+3]-shape[i].y_offset+k)*(board->width)+piece_info[i*4+2]+l] == 1) {
            send(conn_fd, "E 303", 5, 0);
            for(int j = 0; j < 5; j++) free(shape[j].shape);
            return 0;
          }
          board->board[(piece_info[i*4+3]-shape[i].y_offset+k)*(board->width)+piece_info[i*4+2]+l] = 1;
        }
        l++;
      }
      k++;
    }
  }
  for(int j = 0; j < 5; j++) free(shape[j].shape);
  return 1;
}

// Handles the 'I' packets
int initialize_game(Board *board, int conn_fd, char *buffer) {
  int done = 0;
  char packet_type;
  char extra[1024];
  int args[5][4];
  while(!done) {
    read_fd(conn_fd, buffer);
    if(sscanf(buffer, "%c %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %s", &packet_type, &args[0][0], &args[0][1], &args[0][2], &args[0][3],  &args[1][0], &args[1][1], &args[1][2], &args[1][3], &args[2][0], &args[2][1], &args[2][2], &args[2][3], &args[3][0], &args[3][1], &args[3][2], &args[3][3], &args[4][0], &args[4][1], &args[4][2], &args[4][3], extra) == 21 && packet_type == 'I') {
      if(initialize_board(board, (int*)args, conn_fd) != 0) {
        done = 1;
        send(conn_fd, "A", 1, 0);
      }
      else memset(board->board, 0, board->width*board->height*sizeof(int));
    }
    else if(packet_type == 'F') return 1;
    else if(packet_type != 'I') send(conn_fd, "101", 3, 0);
    else send(conn_fd, "E 201", 5, 0);
  }
  return 0;
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

  int forfeit;
  struct Board board1, board2;
  char player1_history[1024], board2_history[1024];
  player1_history[0] = '\0';
  board2_history[0] = '\0';

  // Playing the game
  if((forfeit = begin_game(&board1, &board2, conn_fd1, conn_fd2, buffer)) != 0) handle_forfeit(conn_fd1, conn_fd2, forfeit, buffer);
  if((forfeit = initialize_game(&board1, conn_fd1, buffer)) != 0) handle_forfeit(conn_fd1, conn_fd2, 1, buffer);
  if((forfeit = initialize_game(&board2, conn_fd2, buffer)) != 0) handle_forfeit(conn_fd1, conn_fd2, 2, buffer);
  for(int i = 0; i < board1.height; i++) {
    for(int j = 0; j < board1.width; j++) {
      printf("%d ", board1.board[i*board1.width+j]);
    }
    printf("\n");
  }
  // Server Shutdown
  printf("[Server] Shutting down.\n");
  free(board1.board);
  free(board2.board);
  close(conn_fd1);
  close(listen_fd1);
  close(conn_fd2);
  close(listen_fd2);
  return EXIT_SUCCESS;
}
