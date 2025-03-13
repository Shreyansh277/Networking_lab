// wordserver.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#define BUFFER_SIZE 1024

void handle_client(int sockfd, struct sockaddr_in *client_addr, socklen_t addr_len) {
    char buffer[BUFFER_SIZE];
    char filename[BUFFER_SIZE];
    FILE *file;

    // Receive filename from client
    recvfrom(sockfd, filename, BUFFER_SIZE, 0, (struct sockaddr *)client_addr, &addr_len);
    filename[strcspn(filename, "\n")] = 0; // Remove newline

    file = fopen(filename, "r");
    if (!file) {
        sprintf(buffer, "NOTFOUND %s", filename);
        sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)client_addr, addr_len);
        return;
    }

    fgets(buffer, BUFFER_SIZE, file); // Read HELLO
    sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)client_addr, addr_len);

    while (fgets(buffer, BUFFER_SIZE, file)) {
        sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)client_addr, addr_len);
        if (strstr(buffer, "FINISH")) break;
        usleep(50000); // Simulate delay
    }
    fclose(file);
}

int main() {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Server running on port 8080\n");

    while (1) {
        handle_client(sockfd, &client_addr, addr_len);
    }

    close(sockfd);
    return 0;
}
