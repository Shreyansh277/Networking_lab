/* Updated ksocket.h */
/* Header file for KTP sockets - Updated for Assignment 4 */

#ifndef KSOCKET_H
#define KSOCKET_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdbool.h>

#define SOCK_KTP 1
#define T 5 // Timeout in seconds
#define MESSAGE_SIZE 512 // Message size in bytes
#define SEQ_NUM_BITS 8 // 8-bit sequence number
#define BUFFER_SIZE 10 // Receiver buffer size in number of messages
#define MAX_KTP_SOCKETS 10 // Maximum number of active KTP sockets

// Error codes
#define ENOSPACE 1001 // No space in the send buffer
#define ENOTBOUND 1002 // Socket not bound
#define ENOMESSAGE 1003 // No message available

// KTP Message Header Structure
struct ktp_header {
    uint8_t seq_num;  // Sequence number (8-bit)
};

// KTP Message Structure
struct ktp_message {
    struct ktp_header header;
    char payload[MESSAGE_SIZE];
};

// Sender Window Structure
struct ktp_swnd {
    uint8_t size; // Sender window size
    uint8_t seq_numbers[BUFFER_SIZE]; // Messages sent but not acknowledged
};

// Receiver Window Structure
struct ktp_rwnd {
    uint8_t size; // Receiver window size
    uint8_t expected_seq; // Next expected sequence number
};

// KTP Socket Structure
struct ktp_socket {
    int udp_sockfd; // Underlying UDP socket
    struct sockaddr_in local_addr; // Local address
    struct sockaddr_in remote_addr; // Remote address
    struct ktp_message send_buffer[BUFFER_SIZE]; // Sender buffer
    struct ktp_message recv_buffer[BUFFER_SIZE]; // Receiver buffer
    struct ktp_swnd swnd; // Sender window
    struct ktp_rwnd rwnd; // Receiver window
    bool active; // Indicates if the socket is active
};

// Function prototypes for KTP sockets
int k_socket(int domain, int type, int protocol);
int k_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
ssize_t k_sendto(int sockfd, const void *buf, size_t len, int flags,
                 const struct sockaddr *dest_addr, socklen_t addrlen);
ssize_t k_recvfrom(int sockfd, void *buf, size_t len, int flags,
                   struct sockaddr *src_addr, socklen_t *addrlen);
int k_close(int sockfd);

// Message loss simulation function
int dropMessage(float p);

#endif
