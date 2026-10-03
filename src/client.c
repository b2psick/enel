#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 9001
#define BUFFER_SIZE 65536
#define MAX_FILENAME 255

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

void encode_size(uint64_t size, unsigned char buffer[8]) {
    for (int i = 7; i >= 0; i--) {
        buffer[i] = size & 0xff;
        size >>= 8;
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <IP> <file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int file_fd = open(argv[2], O_RDONLY);
    if (file_fd == -1) {
        perror("open");
        return EXIT_FAILURE;
    }

    struct stat file_info;
    if (fstat(file_fd, &file_info) == -1 || !S_ISREG(file_info.st_mode) || file_info.st_size < 0) {
        fprintf(stderr, "Invalid file.\n");
        close(file_fd);
        return EXIT_FAILURE;
    }

    uint64_t file_size = (uint64_t)file_info.st_size;
    const char *filename = strrchr(argv[2], '/');
    filename = filename ? filename + 1 : argv[2];
    size_t filename_length = strlen(filename);

    if (filename_length == 0 || filename_length > MAX_FILENAME) {
        fprintf(stderr, "Invalid filename length.\n");
        close(file_fd);
        return EXIT_FAILURE;
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket");
        close(file_fd);
        return EXIT_FAILURE;
    }

    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) != 1) {
        fprintf(stderr, "Invalid IP address.\n");
        close(file_fd);
        close(sockfd);
        return EXIT_FAILURE;
    }

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        close(file_fd);
        close(sockfd);
        return EXIT_FAILURE;
    }

    unsigned char size_buffer[8];
    encode_size(file_size, size_buffer);
    uint16_t name_length = htons((uint16_t)filename_length);

    if (send_all(sockfd, size_buffer, 8) == -1 || send_all(sockfd, &name_length, 2) == -1 || send_all(sockfd, filename, filename_length) == -1) {
        perror("send metadata");
        close(file_fd);
        close(sockfd);
        return EXIT_FAILURE;
    }

    char buffer[BUFFER_SIZE];
    uint64_t total_sent = 0;

    while (total_sent < file_size) {
        size_t to_read = sizeof(buffer);
        if (file_size - total_sent < to_read) to_read = (size_t)(file_size - total_sent);

        ssize_t n = read(file_fd, buffer, to_read);
        if (n < 0) {
            if (errno == EINTR) continue;
            perror("read");
            break;
        }
        if (n == 0) {
            fprintf(stderr, "File changed during transfer.\n");
            break;
        }

        if (send_all(sockfd, buffer, (size_t)n) == -1) {
            perror("send");
            break;
        }

        total_sent += (uint64_t)n;
        printf("\rTransferred: %.2f%%", file_size ? (double)total_sent * 100.0 / file_size : 100.0);
        fflush(stdout);
    }

    printf("\n");

    if (total_sent != file_size) {
        close(file_fd);
        close(sockfd);
        return EXIT_FAILURE;
    }

    unsigned char status;
    if (recv_all(sockfd, &status, 1) == -1 || status != 1) {
        fprintf(stderr, "Server failed to save the file.\n");
        close(file_fd);
        close(sockfd);
        return EXIT_FAILURE;
    }

    printf("File transferred successfully!\n");
    close(file_fd);
    close(sockfd);
    return EXIT_SUCCESS;
}