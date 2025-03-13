// ktp_client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "k_socket.h"

#define END_MARKER "#"
#define BUFFER_SIZE 512
#define MAX_FILENAME_LENGTH 256
#define MAX_IP_LENGTH 16

int main(int argc, char *argv[])
{
    if (argc != 5)
    {
        printf("Usage: %s <src_IP> <src_port> <dest_IP> <dest_port>\n", argv[0]);
        return 1;
    }

    // Initialize KTP protocol
    initialize();

    // Validate and copy IP addresses
    char src_ip[MAX_IP_LENGTH];
    char dest_ip[MAX_IP_LENGTH];

    if (strlen(argv[1]) >= MAX_IP_LENGTH || strlen(argv[3]) >= MAX_IP_LENGTH)
    {
        printf("Error: IP address too long\n");
        return 1;
    }

    strncpy(src_ip, argv[1], MAX_IP_LENGTH - 1);
    strncpy(dest_ip, argv[3], MAX_IP_LENGTH - 1);
    src_ip[MAX_IP_LENGTH - 1] = '\0';
    dest_ip[MAX_IP_LENGTH - 1] = '\0';

    // Convert ports
    int src_port = atoi(argv[2]);
    int dest_port = atoi(argv[4]);

    if (src_port <= 0 || dest_port <= 0)
    {
        printf("Error: Invalid port numbers\n");
        return 1;
    }

    // Create socket
    pid_t process_id = getpid();
    int ktp_sockfd = k_socket(AF_INET, SOCK_DGRAM, 0, process_id);

    if (ktp_sockfd < 0)
    {
        printf("Error in socket creation: %d\n", ktp_sockfd);
        return 1;
    }

    // Set up addresses
    struct sockaddr_in servaddr, cliaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    // Setup server address (destination)
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(dest_port);
    if (inet_aton(dest_ip, &servaddr.sin_addr) == 0)
    {
        printf("Error: Invalid destination IP address\n");
        k_close(ktp_sockfd, NULL);
        return 1;
    }

    // Setup client address (source)
    cliaddr.sin_family = AF_INET;
    cliaddr.sin_port = htons(src_port);
    if (inet_aton(src_ip, &cliaddr.sin_addr) == 0)
    {
        printf("Error: Invalid source IP address\n");
        k_close(ktp_sockfd, NULL);
        return 1;
    }

    // Bind socket
    if (k_bind(ktp_sockfd, (struct sockaddr *)&cliaddr, (struct sockaddr *)&servaddr) < 0)
    {
        printf("Error in binding\n");
        k_close(ktp_sockfd, NULL);
        return 1;
    }

    // Get filename from user
    char filename[MAX_FILENAME_LENGTH];
    printf("Enter filename: ");
    if (fgets(filename, MAX_FILENAME_LENGTH, stdin) == NULL)
    {
        printf("Error reading filename\n");
        k_close(ktp_sockfd, NULL);
        return 1;
    }
    filename[strcspn(filename, "\n")] = 0; // Remove newline

    // Open and send file
    int fd;
    while ((fd = open(filename, O_RDONLY)) < 0)
    {
        printf("File not found\n");
        printf("Enter filename: ");
        if (fgets(filename, MAX_FILENAME_LENGTH, stdin) == NULL)
        {
            printf("Error reading filename\n");
            k_close(ktp_sockfd, NULL);
            return 1;
        }
        filename[strcspn(filename, "\n")] = 0;
    }

    // First send the filename
    char file_header[MAX_FILENAME_LENGTH + 8] = "FILE:";
    strcat(file_header, filename);
    if (k_sendto(ktp_sockfd, file_header, strlen(file_header), 0, (struct sockaddr *)&servaddr) < 0)
    {
        printf("Error sending filename\n");
        close(fd);
        k_close(ktp_sockfd, NULL);
        return 1;
    }

    // Send file contents
    char buffer[BUFFER_SIZE];
    int readbytes;
    int total_bytes = 0;

    while ((readbytes = read(fd, buffer, BUFFER_SIZE - 1)) > 0)
    {
        buffer[readbytes] = '\0';
        //printf ("sending to ktpscoket %s\n", buffer);
        int sent = k_sendto(ktp_sockfd, buffer, readbytes, 0, (struct sockaddr *)&servaddr);
        if (sent < 0)
        {
            printf("Error sending file data\n");
            close(fd);
            k_close(ktp_sockfd, NULL);
            return 1;
        }
        total_bytes += sent;
        printf("Sent %d bytes\r", total_bytes);
        fflush(stdout);
    }
    printf("\n");

    // Send end marker
    if (k_sendto(ktp_sockfd, END_MARKER, 1, 0, (struct sockaddr *)&servaddr) < 0)
    {
        printf("Error sending end marker\n");
        close(fd);
        k_close(ktp_sockfd, NULL);
        return 1;
    }

    printf("File transfer completed successfully\n");
    close(fd);
    k_close(ktp_sockfd, NULL);
    return 0;
}