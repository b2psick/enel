#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 9001
#define BUFFER_SIZE 1024

int main() {
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);

    if(client_fd == -1) {
        perror("socket");
        return EXIT_FAILURE;
    }

    struct sockaddr_in server_addr = {0};

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    if(inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) != 1) {
        perror("inet_pton");
        close(client_fd);
        return EXIT_FAILURE;
    }

    if(connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        close(client_fd);
        return EXIT_FAILURE;
    }

    const char *message = "hello from client";

    if(send(client_fd, message, strlen(message), 0) == -1) {
        perror("send");
        close(client_fd);
        return EXIT_FAILURE;
    }

    char buffer[BUFFER_SIZE];

    ssize_t bytes_recieved = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

    if(bytes_recieved == -1) {
        perror("recv");
        close(client_fd);
        return EXIT_FAILURE;
    }

    buffer[bytes_recieved] = '\0';

    printf("server says: %s\n", buffer);

    close(client_fd);

    return EXIT_SUCCESS;

}