// ktp.h
#ifndef KTP_H
#define KTP_H

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include<sys/select.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#define MAX_SEQ 255
#define PORT 5000
#define MAXLINE 1000
#define N 10                  // Maximum number of KTP sockets
#define P 0.5                 // Probability of message drop
#define T 10                  // Timeout in microseconds
#define SENDBUFFERSIZE 10
#define RECEIVEBUFFERSIZE 10
#define MAX_MSG_SIZE 1024

// Error codes
#define ENOTBOUND -2
#define ENOSPACE -3
#define ENOMSG -4
#define INVALIDSOCKET -5 
#define BINDFAILED -6
#define SENDFAILED -7
#define RECVFAILED -8

// Message structure
struct message {
    char msg[MAX_MSG_SIZE];
    int ktp_socket_id;
};

// KTP socket structure
struct KTP_socket {
    bool isFree;
    int processId;
    int udpSocketId;
    char ipAddress[INET_ADDRSTRLEN];
    int port;

    int send_seq_num; //sequence number while sending packets
    int recv_seq_num; //sequence number  of expected recieve
    struct message send_buffer[SENDBUFFERSIZE];
    int send_buffer_pointer;
    struct message receive_buffer[RECEIVEBUFFERSIZE];
    int receive_buffer_pointer;
};

// Sent message structure for reliability
struct sent_message {
    struct message msg;
    time_t send_time;
};

// Function declarations
void initialize(void);
int k_socket(int family, int protocol, int flag, int processID);
int k_bind(int ktp_socketId, struct sockaddr *source, struct sockaddr *destination);
int k_sendto(int ktp_sockfd, char* mess, int maxSize, int flag, struct sockaddr* destination);
int k_recvfrom(int ktp_sockfd, char *message, int maxSize, int flag, struct sockaddr *source);
void k_close(int sockfd, struct sockaddr *dest);

#endif // KTP_H