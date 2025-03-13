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
    char filename[] = "ff.txt";  // Replace with your roll number
    int sockfd;
    struct sockaddr_in servaddr;

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

    // Send filename to server
    sendto(sockfd, filename, strlen(filename), 0, (struct sockaddr*)&servaddr, sizeof(servaddr));

    // Receive server response
    int n = recvfrom(sockfd, buffer, MAXLINE, 0, NULL, NULL);
    buffer[n] = '\0';

    // Check for file not found
    if (strncmp(buffer, "NOTFOUND", 8) == 0) {
        printf("FILE NOT FOUND\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Open output file
    FILE *outfile = fopen("received_file.txt", "w");
    if (!outfile) {
        perror("Cannot create output file");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Receive and write file contents
    while (1) {
        // Acknowledge previous word and request next
        sendto(sockfd, "WORD", 4, 0, (struct sockaddr*)&servaddr, sizeof(servaddr));

        // Check for end of file
        if (strcmp(buffer, "FINISH") == 0) {
            fprintf(outfile, "%s\n", buffer);
            break;
        }

        // Write word to file
        fprintf(outfile, "%s\n", buffer);
        // Receive word
        n = recvfrom(sockfd, buffer, MAXLINE, 0, NULL, NULL);
        buffer[n] = '\0';
        

    }

    fclose(outfile);
    close(sockfd);
    return 0;
}