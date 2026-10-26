#include <arpa/inet.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main(int argc, char const *argv[]) {
  int sock = 0;
  struct sockaddr_in serv_addr;
  char filename[256] = {0};

  if (argc != 2) {
    fprintf(stderr, "Usage: %s <server_ip>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  // Create client socket
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket creation failed");
    exit(EXIT_FAILURE);
  }

  // Config server address
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(PORT);

  // Convert server IP from text to binary form
  if (inet_pton(AF_INET, argv[1], &serv_addr.sin_addr) <= 0) {
    perror("invalid server IP address");
    exit(EXIT_FAILURE);
  }

  // Connect to server
  if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    perror("connection failed");
    exit(EXIT_FAILURE);
  }
  printf("Connected to server %s:%d\n", argv[1], PORT);

  // Get filename from user and send to server
  printf("Enter filename to request: ");
  fgets(filename, sizeof(filename), stdin);
  filename[strcspn(filename, "\n")] = '\0'; // Remove newline character

  if (send(sock, filename, strlen(filename), 0) < 0) {
    perror("send filename failed");
    exit(EXIT_FAILURE);
  }

  // Receive file size from server
  long network_file_size;
  ssize_t valread = recv(sock, &network_file_size, sizeof(network_file_size), 0);
  if (valread < 0) {
    perror("recv filesize failed");
    close(sock);
    exit(EXIT_FAILURE);
  }

  long file_size = ntohl(network_file_size);
  if (file_size == 0) {
    printf("Error: File '%s' not found on server.\n", filename);
    close(sock);
    exit(EXIT_FAILURE);
  }
  printf("Receiving file: %s (siz %ld bytes)\n", filename, file_size);

  // Create local file to save receive data
  FILE *file = fopen(filename, "wb"); // Open in binary write mode
  if (!file) {
    perror("failed to create file");
    close(sock);
    exit(EXIT_FAILURE);
  }

  // Receive file data in chunks and write to file
  char buffer[BUFFER_SIZE];
  long remaining = file_size;
  while (remaining > 0) {
    size_t bytes_to_read = (remaining < BUFFER_SIZE) ? remaining : BUFFER_SIZE;
    valread = recv(sock, buffer, bytes_to_read, 0);
    if (valread < 0) {
      perror("recv file data failed");
      fclose(file);
      close(sock);
      exit(EXIT_FAILURE);
    }
    fwrite(buffer, 1, valread, file);
    remaining -= valread;
    printf("Received %ld/%ld bytes\n", file_size - remaining, file_size);
    fflush(stdout);
  }
  printf("\nFile received successfully! Saved as: %s\n", filename);


  // Cleanup
  fclose(file);
  close(sock);
  return 0;
}
