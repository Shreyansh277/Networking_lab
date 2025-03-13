// ktp.c
#include "k_socket.h"
// #include <sys/select.h>

// Global variables
static struct KTP_socket KTP_sockets[N];
static struct sent_message *send_buffer = NULL;
static int send_buffer_size = 0;
static int send_pointer = 0;
static int global_error = 0;

static int dropMessage(float p)
{
    float r = (float)rand() / (float)RAND_MAX;
    return (r >= p);
}

static void SendMsg(struct message *Message)
{
    if (!dropMessage(P))
    {
        printf("[DROP] Message dropped (probability check)\n");
        return;
    }

    struct KTP_socket *ktp_socket = &KTP_sockets[Message->ktp_socket_id];
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));

    servaddr.sin_addr.s_addr = inet_addr(ktp_socket->ipAddress);
    servaddr.sin_port = htons(ktp_socket->port);
    servaddr.sin_family = AF_INET;

    char buffer[MAX_MSG_SIZE + 1];
    buffer[0] = 'D';
    buffer[1] = '0' + ktp_socket->send_seq_num;
    strncpy(buffer + 2, Message->msg, MAX_MSG_SIZE - 2);

    sendto(ktp_socket->udpSocketId, buffer, strlen(buffer) + 1, 0,
           (struct sockaddr *)&servaddr, sizeof(servaddr));

  printf("[SUCCESSFUL UDP SEND] Message sent to IP %s port %d:\n",ktp_socket->ipAddress, ktp_socket->port);
}
void initialize(void)
{
    for (int i = 0; i < N; i++)
    {
        KTP_sockets[i].isFree = true;
        KTP_sockets[i].processId = -1;
        KTP_sockets[i].udpSocketId = -1;
        KTP_sockets[i].port = -1;
        KTP_sockets[i].send_seq_num=1;
        KTP_sockets[i].recv_seq_num=1;
        KTP_sockets[i].send_buffer_pointer = 0;
        KTP_sockets[i].receive_buffer_pointer = 0;
    }

    // Initialize send buffer
    send_buffer = malloc(sizeof(struct sent_message) * SENDBUFFERSIZE);
    if (send_buffer == NULL)
    {
        fprintf(stderr, "Failed to allocate send buffer\n");
        exit(1);
    }
}

int k_socket(int family, int protocol, int flag, int processID)
{
    int free_slot = -1;
    for (int i = 0; i < N; i++)
    {
        if (KTP_sockets[i].isFree)
        {
            free_slot = i;
            break;
        }
    }

    if (free_slot == -1)
    {
        return ENOSPACE;
    }

    int socketId = socket(family, protocol, flag);
    if (socketId < 0)
    {
        return INVALIDSOCKET;
    }

    KTP_sockets[free_slot].udpSocketId = socketId;
    KTP_sockets[free_slot].isFree = false;
    KTP_sockets[free_slot].processId = processID;

    return free_slot;
}

int k_bind(int ktp_socketId, struct sockaddr *source, struct sockaddr *destination)
{
    if (ktp_socketId < 0 || ktp_socketId >= N || KTP_sockets[ktp_socketId].isFree)
    {
        return INVALIDSOCKET;
    }

    if (bind(KTP_sockets[ktp_socketId].udpSocketId, source, sizeof(*source)) < 0)
    {
        return BINDFAILED;
    }

    // Store destination address
    struct sockaddr_in *dest = (struct sockaddr_in *)destination;
    strncpy(KTP_sockets[ktp_socketId].ipAddress,
            inet_ntoa(dest->sin_addr),
            INET_ADDRSTRLEN);
    KTP_sockets[ktp_socketId].port = ntohs(dest->sin_port);

    return 0;
}

int k_sendto(int ktp_sockfd, char *mess, int maxSize, int flag, struct sockaddr *destination)
{
    if (ktp_sockfd < 0 || ktp_sockfd >= N || KTP_sockets[ktp_sockfd].isFree)
    {
        return INVALIDSOCKET;
    }

    struct message m;
    strncpy(m.msg, mess, MAX_MSG_SIZE - 1);
    m.msg[MAX_MSG_SIZE - 1] = '\0';
    m.ktp_socket_id = ktp_sockfd;

    // Add to socket's send buffer
    if (KTP_sockets[ktp_sockfd].send_buffer_pointer < SENDBUFFERSIZE)
    {
        KTP_sockets[ktp_sockfd].send_buffer[KTP_sockets[ktp_sockfd].send_buffer_pointer++] = m;
    }

    // Add to global send buffer
    if (send_buffer_size < SENDBUFFERSIZE)
    {
        send_buffer[send_buffer_size].msg = m;
        send_buffer[send_buffer_size].send_time = time(NULL);
        send_buffer_size++;
    }

    //printf("\n\n[SEND] Attempting to send message to port %d the message %s\n", KTP_sockets[ktp_sockfd].port,mess);
    SendMsg(&m);

    int max_retries = 50; // Maximum number of retries
    int retry_count = 0;

    while (retry_count < max_retries)
    {
        // Reset timeout for each attempt
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = T;

        fd_set readfds;
        char ack_buffer[2];
        struct sockaddr_in sender_addr;
        socklen_t sender_len = sizeof(sender_addr);

        FD_ZERO(&readfds);
        FD_SET(KTP_sockets[ktp_sockfd].udpSocketId, &readfds);

        int activity = select(KTP_sockets[ktp_sockfd].udpSocketId + 1, &readfds, NULL, NULL, &tv);

        if (activity == 0)
        {
            // Timeout occurred
            retry_count++;
            if (retry_count < max_retries)
            {
                printf("[TIMEOUT] No acknowledgment received, number of retires retry %d \n", retry_count);
                printf("[RESEND] Resending message to port %d\n", KTP_sockets[ktp_sockfd].port);
                SendMsg(&m);
                continue;
            }
            else
            {
                printf("[FAILED] Maximum retries exceeded, giving up\n");
                return SENDFAILED;
            }
        }

        if (activity < 0)
        {
            printf("[ERROR] Select failed\n");
            return RECVFAILED;
        }

        int received = recvfrom(KTP_sockets[ktp_sockfd].udpSocketId,
                                ack_buffer, 2, 0,
                                (struct sockaddr *)&sender_addr, &sender_len
                            );

        
       
                                

        if (received > 0 && ack_buffer[0] == 'A')
        {
            printf("[ACK] Received acknowledgment from port %d\n", ntohs(sender_addr.sin_port));
            KTP_sockets[ktp_sockfd].send_seq_num = (KTP_sockets[ktp_sockfd].send_seq_num + 1) % 10; //for now since only one message ,10 sequance number works
            //will change it at final version
            send_buffer_size--;
            return strlen(m.msg);
        }
    }

    return SENDFAILED;
}

int k_recvfrom(int ktp_sockfd, char *message, int maxSize, int flag, struct sockaddr *source)
{
    if (ktp_sockfd < 0 || ktp_sockfd >= N || KTP_sockets[ktp_sockfd].isFree)
    {
        return INVALIDSOCKET;
    }

    char buffer[MAX_MSG_SIZE + 1];
    struct sockaddr_in sender_addr;
    socklen_t sender_len = sizeof(sender_addr);
    int maxRetries = 10; // Avoid infinite loops

    printf("[RECV] Waiting for message on socket %d (port %d)\n",
           ktp_sockfd, KTP_sockets[ktp_sockfd].port);

    while (maxRetries--)
    {
        int received = recvfrom(KTP_sockets[ktp_sockfd].udpSocketId,
                                buffer, MAX_MSG_SIZE, 0,
                                (struct sockaddr *)&sender_addr, &sender_len);

        if (received <= 0)
        {
            printf("[ERROR] Failed to receive message\n");
            return RECVFAILED;
        }

        buffer[received] = '\0';
        printf("[RECV] Received message from port %d: \n",
               ntohs(sender_addr.sin_port));

        if (buffer[0] == 'D')
        {
            int seq_num = atoi(buffer + 1);

            printf("[INFO] Sequence received from port %d: %d (Expected: %d)\n",
                   ntohs(sender_addr.sin_port), seq_num, KTP_sockets[ktp_sockfd].recv_seq_num);

            if (seq_num == KTP_sockets[ktp_sockfd].recv_seq_num)
            {
                // Valid new packet, send acknowledgment
                char ack = 'A';
                sendto(KTP_sockets[ktp_sockfd].udpSocketId, &ack, 1, 0,
                       (struct sockaddr *)&sender_addr, sender_len);

                printf("[ACK] Sent acknowledgment to port %d\n", ntohs(sender_addr.sin_port));

                // Copy message content (excluding 'D' prefix)
                strncpy(message, buffer + 2, maxSize - 1);
                message[maxSize - 1] = '\0';

                // Increment sequence number
                KTP_sockets[ktp_sockfd].recv_seq_num = (KTP_sockets[ktp_sockfd].recv_seq_num + 1) % 10;

                return strlen(message);
            }
            else
            {
                printf("[DROP] Duplicate message from port %d. Ignoring...\n", ntohs(sender_addr.sin_port));
                continue; // Wait for a new packet
            }
        }
        else
        {
            printf("[WARNING] Received non-data message\n");
            return ENOMSG;
        }
    }

    printf("[ERROR] Max retries reached while waiting for a new message.\n");
    return RECVFAILED;
}

void k_close(int sockfd, struct sockaddr *dest)
{
    for (int i = 0; i < N; i++)
    {
        if (!KTP_sockets[i].isFree && KTP_sockets[i].udpSocketId == sockfd)
        {
            close(sockfd);
            KTP_sockets[i].isFree = true;
            KTP_sockets[i].processId = -1;
            KTP_sockets[i].udpSocketId = -1;
            KTP_sockets[i].port = -1;
            KTP_sockets[i].send_buffer_pointer = 0;
            KTP_sockets[i].receive_buffer_pointer = 0;
            break;
        }
    }
    return;
}