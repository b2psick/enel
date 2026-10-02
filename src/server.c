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

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if(server_fd == -1) {
        perror("socket");
        return EXIT_FAILURE;
    }

    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);

    if(bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_fd);
        return EXIT_FAILURE;
    }

    if(listen(server_fd, 10) == -1) {
        perror("listen");
        close(server_fd);
        return EXIT_FAILURE;
    }

    printf("server listening on port %d...\n", PORT);

    int client_fd = accept(server_fd, NULL, NULL);

    if(client_fd == -1) {
        perror("accept");
        close(server_fd);
        return EXIT_FAILURE;
    }

    printf("client connected.\n");

    char buffer[BUFFER_SIZE];

    ssize_t bytes_recieved = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

    if(bytes_recieved == -1) {
        perror("recv");
        close(client_fd);
        close(server_fd);
        return EXIT_FAILURE;
    }

    buffer[bytes_recieved] = '\0';

    printf("client says: %s\n", buffer);

    const char* response = "hello from server!";

    if(send(client_fd, response, strlen(response), 0) == -1) {
        perror("send");
        close(client_fd);
        close(server_fd);
        return EXIT_FAILURE;
    }

    close(client_fd);
    close(server_fd);

    return EXIT_SUCCESS;

}