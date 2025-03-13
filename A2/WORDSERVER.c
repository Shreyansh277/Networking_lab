/*
NAME - ANSH SAHU
ROLL - 22CS30010
ASSIGNMENT - 2
PCAP FILE LINK - https://drive.google.com/drive/u/1/folders/11lfpom1JluA0xnMkYXck5-wx7gStQilR
*/
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>

#define PORT 5000
#define MAXLINE 1024

int main() {
    char buffer[MAXLINE];
    int sockfd;
    struct sockaddr_in servaddr, client_addr;
    socklen_t len;
    FILE *file;

    // Create socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Configure server address
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);
    servaddr.sin_family = AF_INET;

    // Bind socket
    if (bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    printf("Server Running .........\n");

    // Receive filename
    len = sizeof(client_addr);
    int n = recvfrom(sockfd, buffer, MAXLINE, 0, (struct sockaddr*)&client_addr, &len);
    buffer[n] = '\0';

    // Check file existence
    file = fopen(buffer, "r");
    if (!file) {
        char *notfound="NOTFOUND";

        sendto(sockfd, notfound, strlen(notfound), 0, (struct sockaddr*)&client_addr, sizeof(client_addr));
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Read and send file contents word by word
    while (fgets(buffer, MAXLINE, file)) {
        // Remove newline if present
        buffer[strcspn(buffer, "\n")] = 0;
        
        sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr*)&client_addr, sizeof(client_addr));
        
        // Wait for client acknowledgment
        recvfrom(sockfd, buffer, MAXLINE, 0, (struct sockaddr*)&client_addr, &len);
    }

    fclose(file);
    close(sockfd);
    return 0;
}
