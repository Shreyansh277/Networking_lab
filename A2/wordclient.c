
// wordclient.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#define BUFFER_SIZE 1024

int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    char* filename;
    FILE *file;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    filename = "f.txt";

    sendto(sockfd, filename, strlen(filename), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));

    file = fopen(filename, "w");
    if (!file) {
        perror("Error creating file");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    while (1) {
        recvfrom(sockfd, buffer, BUFFER_SIZE, 0, NULL, NULL);
        if (strstr(buffer, "NOTFOUND")) {
            printf("File not found on server.\n");
            break;
        }
        fputs(buffer, file);
        if (strstr(buffer, "FINISH")) break;
    }

    printf("File received successfully.\n");
    fclose(file);
    close(sockfd);
    return 0;
}
