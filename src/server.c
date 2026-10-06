#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "config.h"

int recv_all(int fd, void *data, size_t length) {
    char *ptr = data;
    size_t total = 0;

    while (total < length) {
        ssize_t n = recv(fd, ptr + total, length - total, 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (n == 0) return -1;
        total += n;
    }
    return 0;
}

int send_all(int fd, const void *data, size_t length) {
    const char *ptr = data;
    size_t total = 0;

    while (total < length) {
        ssize_t n = send(fd, ptr + total, length - total, MSG_NOSIGNAL);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (n == 0) return -1;
        total += n;
    }
    return 0;
}

uint64_t decode_size(const unsigned char buffer[8]) {
    uint64_t size = 0;
    for (int i = 0; i < 8; i++) size = (size << 8) | buffer[i];
    return size;
}

int main(void) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        return EXIT_FAILURE;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_fd);
        return EXIT_FAILURE;
    }

    if (listen(server_fd, 10) == -1) {
        perror("listen");
        close(server_fd);
        return EXIT_FAILURE;
    }

    printf("Waiting for connections on port %d...\n", PORT);

    int client_fd = accept(server_fd, NULL, NULL);
    if (client_fd == -1) {
        perror("accept");
        close(server_fd);
        return EXIT_FAILURE;
    }

    printf("Client connected!\n");

    unsigned char size_buffer[8];
    uint16_t name_length;

    if (recv_all(client_fd, size_buffer, 8) == -1 || recv_all(client_fd, &name_length, 2) == -1) {
        fprintf(stderr, "Failed to receive metadata.\n");
        close(client_fd);
        close(server_fd);
        return EXIT_FAILURE;
    }

    uint64_t file_size = decode_size(size_buffer);
    name_length = ntohs(name_length);

    if (name_length == 0 || name_length > MAX_FILENAME) {
        fprintf(stderr, "Invalid filename length.\n");
        close(client_fd);
        close(server_fd);
        return EXIT_FAILURE;
    }

    char filename[MAX_FILENAME + 1] = {0};
    if (recv_all(client_fd, filename, name_length) == -1) {
        fprintf(stderr, "Failed to receive filename.\n");
        close(client_fd);
        close(server_fd);
        return EXIT_FAILURE;
    }

    if (strchr(filename, '/') || strchr(filename, '\\') || strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0 || strcspn(filename, "\r\n\t") != strlen(filename)) {
        fprintf(stderr, "Unsafe filename.\n");
        close(client_fd);
        close(server_fd);
        return EXIT_FAILURE;
    }

    printf("Receiving: %s\n", filename);
    printf("File size: %llu bytes\n", (unsigned long long)file_size);

    char path[300];
    snprintf(path, sizeof(path), "downloads/%s", filename);

    int file_fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (file_fd == -1) {
        perror("open output file");
        close(client_fd);
        close(server_fd);
        return EXIT_FAILURE;
    }

    char buffer[BUFFER_SIZE];
    uint64_t total_received = 0;
    int success = 1;

    while (total_received < file_size) {
        size_t to_read = sizeof(buffer);
        if (file_size - total_received < to_read) to_read = (size_t)(file_size - total_received);

        ssize_t n = recv(client_fd, buffer, to_read, 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            perror("recv");
            success = 0;
            break;
        }
        if (n == 0) {
            fprintf(stderr, "Connection closed unexpectedly.\n");
            success = 0;
            break;
        }

        size_t written = 0;
        while (written < (size_t)n) {
            ssize_t w = write(file_fd, buffer + written, (size_t)n - written);
            if (w < 0) {
                if (errno == EINTR) continue;
                perror("write");
                success = 0;
                break;
            }
            if (w == 0) {
                success = 0;
                break;
            }
            written += (size_t)w;
        }

        if (!success) break;

        total_received += (uint64_t)n;
        printf("\rReceived: %.2f%%", file_size ? (double)total_received * 100.0 / file_size : 100.0);
        fflush(stdout);
    }

    printf("\n");

    if (close(file_fd) == -1) success = 0;

    unsigned char status = success ? 1 : 0;
    send_all(client_fd, &status, 1);

    if (success) printf("File saved: %s\n", path);
    else printf("Transfer failed.\n");

    close(client_fd);
    close(server_fd);
    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}