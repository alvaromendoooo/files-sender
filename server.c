#include <asm-generic/socket.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>

#define BUFFER_SIZE 1024
#define PORT 8080

int main() {
  int server_fd, new_socket;
  struct sockaddr_in address;
  int opt = 1;
  int addrlen = sizeof(address);
  char filename[256] = {0}; // Buffer to store filename from client

  // Creation of socket file description
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    perror("socket creation failed");
    exit(EXIT_FAILURE);
  }

  // Set socket options to reuse port/addeess (avoid address in use errors)
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
    perror("setsockopt");
    exit(EXIT_FAILURE);
  }

  // Configure server address
  address.sin_family = AF_INET; // IPv4
  address.sin_addr.s_addr = INADDR_ANY; // Listen on all interfaces
  address.sin_port = htons(PORT); // PORT (host-to-network short)

  // Bind socket to port 8080
  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    perror("bind failed");
    exit(EXIT_FAILURE);
  }

  // Listen to incoming connections, max retries 3
  if (listen(server_fd, 3) < 0) {
    perror("listen");
    exit(EXIT_FAILURE);
  }

  // Accept client connections
  if ((new_socket = accept(server_fd, (struct sockaddr* )&address, (socklen_t*)&addrlen)) < 0) {
    perror("accept");
    exit(EXIT_FAILURE);
  }
  printf("Client connected: %s:%d\n", inet_ntoa(address.sin_addr), ntohl(address.sin_port));

  // Receive filename from client
  size_t valread = recv(server_fd, filename, sizeof(filename), 0);
  if (valread < 0) {
    perror("recv failed");
    close(new_socket);
    exit(EXIT_FAILURE);
  }
  printf("Client requested file: %s\n", filename);

  // Open requested file
  FILE *file = fopen(filename, "rb"); // Open in binary read mode
  if (!file) {
    perror("File not found");
    long file_size = 0;
    send(new_socket, &file_size, sizeof(file_size), 0);
    close(new_socket);
    exit(EXIT_FAILURE);
  }

  // Get file size
  struct stat file_stat;
  fstat(fileno(file), &file_stat);
  long file_size = file_stat.st_size;
  printf("Sending file size: %ld bytes\n", file_size);

  // Send file data in chunks
  char buffer[BUFFER_SIZE];
  size_t bytes_read;
  while((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
    if (send(new_socket, buffer, bytes_read, 0) != bytes_read) {
      perror("send file");
      fclose(file);
      close(new_socket);
      exit(EXIT_FAILURE);
    }
  }

  printf("File sent successfully\n");
  // Cleanup
  fclose(file);
  close(new_socket);
  close(server_fd);
  return 0;
}

