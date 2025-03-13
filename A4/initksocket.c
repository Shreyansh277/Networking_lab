#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/select.h>
#include <signal.h>
#include "ksocket.h"
#include <assert.h>

int numOfTransmissions = 0;

/*
    Each message will have a 9 bit header. 1 bit for type (0 = ACK, 1 = DATA), 8 bit for sequence number.
    Next bits are either 3-bit rwnd size (for ACK) or 10 bits of length and 512 byte data (for DATA).
*/

void handler(int sig)
{
    // release resources
    shmdt(sock_info);
    shmdt(SM);
    shmctl(shmid_sock_info, IPC_RMID, NULL);
    shmctl(shmid_SM, IPC_RMID, NULL);
    semctl(sem1, 0, IPC_RMID);
    semctl(sem2, 0, IPC_RMID);
    semctl(sem_SM, 0, IPC_RMID);
    semctl(sem_sock_info, 0, IPC_RMID);
    exit(0);
}

void *R()
{
    fd_set readfds;
    FD_ZERO(&readfds);
    int nfds = 0;
    while (1)
    {
        fd_set readyfds = readfds;
        struct timeval tv;
        tv.tv_sec = T;
        tv.tv_usec = 0;
        int retval = select(nfds + 1, &readyfds, NULL, NULL, &tv);

        if (retval == -1)
        {
            perror("select()");
        }
        if (retval <= 0)
        {
            // Timeout
            FD_ZERO(&readfds);
            nfds = 0;
            P(sem_SM);
            for (int i = 0; i < N; i++)
            {
                if (SM[i].is_free == 0)
                {
                    // Add the socket to the readfds
                    FD_SET(SM[i].udp_socket_id, &readfds);
                    if (SM[i].udp_socket_id > nfds)
                        nfds = SM[i].udp_socket_id;

                    // Check if the receive window has space now
                    if (SM[i].nospace && SM[i].rwnd.size > 0)
                    {
                        SM[i].nospace = 0;
                    }

                    // sending ack regardless of nospace
                    int lastseq = (SM[i].rwnd.start_seq + 255) % 256; // Last in-order sequence number
                    struct sockaddr_in cliaddr;
                    cliaddr.sin_family = AF_INET;
                    inet_aton(SM[i].ip_address, &cliaddr.sin_addr);
                    cliaddr.sin_port = htons(SM[i].port);

                    char ack[12];
                    ack[0] = '0';
                    // Convert 8-bit sequence number to binary representation
                    for (int j = 0; j < 8; j++)
                    {
                        ack[j + 1] = ((lastseq >> (7 - j)) & 1) + '0';
                    }
                    // 3-bit rwnd size
                    ack[9] = (SM[i].rwnd.size >> 2) % 2 + '0';
                    ack[10] = (SM[i].rwnd.size >> 1) % 2 + '0';
                    ack[11] = (SM[i].rwnd.size) % 2 + '0';
                    sendto(SM[i].udp_socket_id, ack, 12, 0, (struct sockaddr *)&cliaddr, sizeof(cliaddr));
                }
            }
            V(sem_SM);
        }
        else
        {
            P(sem_SM);
            for (int i = 0; i < N; i++)
            {
                if (SM[i].is_free == 0 && FD_ISSET(SM[i].udp_socket_id, &readyfds))
                {
                    // Ready to read from this socket
                    char buffer[540]; // Increased from 512 to accommodate header
                    struct sockaddr_in cliaddr;
                    unsigned int len = sizeof(cliaddr);
                    int n = recvfrom(SM[i].udp_socket_id, buffer, 540, 0, (struct sockaddr *)&cliaddr, &len);
                    if (dropMessage(p)){printf("Dropped message\n"); continue;} // Drop message with probability p
                    if (n < 0)
                    {
                        perror("recvfrom()");
                    }
                    else
                    {
                        if (buffer[0] == '0')
                        {
                            // ACK
                            int seq = 0;
                            // Parse 8-bit sequence number
                            for (int j = 0; j < 8; j++)
                            {
                                seq = (seq << 1) | (buffer[j + 1] - '0');
                            }
                            printf("ACK of Seq Num %d Recived\n",seq);
                            int rwnd = (buffer[9] - '0') * 4 + (buffer[10] - '0') * 2 + (buffer[11] - '0');
                            if (seq >= 0 && seq < 256 && SM[i].swnd.wndw[seq] >= 0)
                            {
                                int j = SM[i].swnd.start_seq;
                                while (j != (seq + 1) % 256)
                                { 
                                    if (j >= 0 && j < 256) {
                                        SM[i].swnd.wndw[j] = -1;
                                        SM[i].lastSendTime[j] = -1;
                                        SM[i].send_buffer_sz++;
                                    }
                                    j = (j + 1) % 256;
                                }
                                SM[i].swnd.start_seq = (seq + 1) % 256;
                            }
                            if (rwnd >= 0 && rwnd <= 7) {
                                SM[i].swnd.size = rwnd;
                            }
                        }
                        else
                        {
                            // DATA
                            int seq = 0;
                            // Parse 8-bit sequence number
                            for (int j = 0; j < 8; j++)
                            {
                                seq = (seq << 1) | (buffer[j + 1] - '0');
                            }
                            printf("Data packet of seq no %d recived\n",seq);
                            int len = 0;
                            // Parse 10-bit length
                            for (int j = 0; j < 10; j++)
                            {
                                len = (len << 1) | (buffer[j + 9] - '0');
                            }
                            
                            // Make sure length is reasonable
                            if (len > 512) len = 512;

                            if (seq == SM[i].rwnd.start_seq && SM[i].rwnd.size > 0)
                            {
                                // In order message - find a valid buffer index
                                int buff_ind = -1;
                                if (seq >= 0 && seq < 256 && SM[i].rwnd.wndw[seq] >= 0 && SM[i].rwnd.wndw[seq] < 10) {
                                    buff_ind = SM[i].rwnd.wndw[seq];
                                    if (buff_ind >= 0 && buff_ind < 10) {
                                        memcpy(SM[i].recv_buffer[buff_ind], buffer + 19, len);
                                        SM[i].recv_buffer_valid[buff_ind] = 1;
                                        SM[i].rwnd.size--;
                                        SM[i].lengthOfMessageReceiveBuffer[buff_ind] = len;
                                    }

                                    // Find the next in order message
                                    while (SM[i].rwnd.start_seq < 256 && 
                                           SM[i].rwnd.wndw[SM[i].rwnd.start_seq] >= 0 && 
                                           SM[i].rwnd.wndw[SM[i].rwnd.start_seq] < 10 && 
                                           SM[i].recv_buffer_valid[SM[i].rwnd.wndw[SM[i].rwnd.start_seq]] == 1)
                                    {
                                        SM[i].rwnd.start_seq = (SM[i].rwnd.start_seq + 1) % 256;
                                    }
                                }
                            }
                            else if (seq >= 0 && seq < 256)
                            {
                                // Keep out of order message if in rwnd and we have space
                                if (SM[i].rwnd.size > 0 && 
                                    SM[i].rwnd.wndw[seq] >= 0 && 
                                    SM[i].rwnd.wndw[seq] < 10 && 
                                    SM[i].recv_buffer_valid[SM[i].rwnd.wndw[seq]] == 0)
                                {
                                    int buff_ind = SM[i].rwnd.wndw[seq];
                                    if (buff_ind >= 0 && buff_ind < 10) {
                                        memcpy(SM[i].recv_buffer[buff_ind], buffer + 19, len);
                                        SM[i].recv_buffer_valid[buff_ind] = 1;
                                        SM[i].rwnd.size--;
                                        SM[i].lengthOfMessageReceiveBuffer[buff_ind] = len;
                                    }
                                }
                            }
                            
                            // Nospace
                            if (SM[i].rwnd.size == 0)
                                SM[i].nospace = 1;

                            // Send ACK - last in-order sequence number
                            int lastseq = (SM[i].rwnd.start_seq + 255) % 256;
                            char ack[12];
                            ack[0] = '0';
                            // Convert 8-bit sequence number to binary representation
                            for (int j = 0; j < 8; j++)
                            {
                                ack[j + 1] = ((lastseq >> (7 - j)) & 1) + '0';
                            }
                            // 3-bit rwnd size (0-7)
                            int rwnd_size = SM[i].rwnd.size;
                            if (rwnd_size > 7) rwnd_size = 7;
                            ack[9] = (rwnd_size >> 2) & 1 + '0';
                            ack[10] = (rwnd_size >> 1) & 1 + '0';
                            ack[11] = (rwnd_size) & 1 + '0';
                            sendto(SM[i].udp_socket_id, ack, 12, 0, (struct sockaddr *)&cliaddr, sizeof(cliaddr));
                        }
                    }
                }
            }
            V(sem_SM);
        }
    }
}

void *S()
{
    while (1)
    {
        sleep(T / 2);
        P(sem_SM);
        for (int i = 0; i < N; i++)
        {
            if (SM[i].is_free == 0)
            {
                struct sockaddr_in serv_addr;
                serv_addr.sin_family = AF_INET;
                serv_addr.sin_port = htons(SM[i].port);
                inet_aton(SM[i].ip_address, &serv_addr.sin_addr);
                int timeout = 0;
                int j = SM[i].swnd.start_seq;
                
                // Check for timeouts
                int end_seq = (SM[i].swnd.start_seq + SM[i].swnd.size) % 256;
                while (j != end_seq)
                {
                    if (j >= 0 && j < 256 && SM[i].lastSendTime[j] != -1 && time(NULL) - SM[i].lastSendTime[j] > T)
                    {
                        timeout = 1;
                        break;
                    }
                    j = (j + 1) % 256;
                }
                
                if (timeout)
                {
                    // Resend all packets in window on timeout
                    j = SM[i].swnd.start_seq;
                    int start = SM[i].swnd.start_seq;
                    int end_seq = (start + SM[i].swnd.size) % 256;
                    
                    while (j != end_seq)
                    {
                        if (j >= 0 && j < 256 && SM[i].swnd.wndw[j] >= 0 && SM[i].swnd.wndw[j] < 10)
                        {
                            char buffer[540];
                            buffer[0] = '1';
                            for (int k = 0; k < 8; k++)
                            {
                                buffer[k + 1] = ((j >> (7 - k)) & 1) + '0';
                            }

                            int buff_idx = SM[i].swnd.wndw[j];
                            if (buff_idx >= 0 && buff_idx < 10) {
                                int len = SM[i].lengthOfMessageSendBuffer[buff_idx];
                                if (len > 512) len = 512;
                                
                                // Convert 10-bit length field
                                for (int k = 0; k < 10; k++)
                                {
                                    buffer[k + 9] = ((len >> (9 - k)) & 1) + '0';
                                }

                                memcpy(buffer + 19, SM[i].send_buffer[buff_idx], len);
                                sendto(SM[i].udp_socket_id, buffer, 19 + len, 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
                                printf("Resending packets of current window\n");
                                numOfTransmissions++;
                                SM[i].lastSendTime[j] = time(NULL);
                            }
                        }
                        j = (j + 1) % 256;
                    }
                }
                else
                {
                    // Send new packets
                    j = SM[i].swnd.start_seq;
                    int start = SM[i].swnd.start_seq;
                    int end_seq = (start + SM[i].swnd.size) % 256;
                    
                    while (j != end_seq)
                    {
                        if (j >= 0 && j < 256 && SM[i].swnd.wndw[j] >= 0 && SM[i].swnd.wndw[j] < 10 && SM[i].lastSendTime[j] == -1)
                        {
                            char buffer[540];
                            buffer[0] = '1';
                            // Convert 8-bit sequence number to binary representation
                            for (int k = 0; k < 8; k++)
                            {
                                buffer[k + 1] = ((j >> (7 - k)) & 1) + '0';
                            }

                            int buff_idx = SM[i].swnd.wndw[j];
                            if (buff_idx >= 0 && buff_idx < 10) {
                                int len = SM[i].lengthOfMessageSendBuffer[buff_idx];
                                if (len > 512) len = 512;
                                
                                // Convert 10-bit length field
                                for (int k = 0; k < 10; k++)
                                {
                                    buffer[k + 9] = ((len >> (9 - k)) & 1) + '0';
                                }

                                memcpy(buffer + 19, SM[i].send_buffer[buff_idx], len);
                                sendto(SM[i].udp_socket_id, buffer, 19 + len, 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
                                numOfTransmissions++;
                                SM[i].lastSendTime[j] = time(NULL);
                            }
                        }
                        j = (j + 1) % 256;
                    }
                }
            }
        }
        V(sem_SM);
        printf("Number of transmissions: %d\n", numOfTransmissions);
    }
}

void *G()
{
    // Garbage collector thread to identify killed messages and close their sockets if not closed already in the SM table
    while (1)
    {
        sleep(T);
        P(sem_SM);
        for (int i = 0; i < N; i++)
        {
            if (SM[i].is_free)
                continue; // Free entry
            if (kill(SM[i].process_id, 0) == 0)
                continue;      // Process still running
            SM[i].is_free = 1; // Process killed, free the entry
        }
        V(sem_SM);
    }
}

int main()
{
    numOfTransmissions = 0;
    signal(SIGINT, handler); // Ctrl+C will release resources and exit

    srand(time(0));

    pop.sem_num = vop.sem_num = 0;
    pop.sem_flg = vop.sem_flg = 0;
    pop.sem_op = -1;
    vop.sem_op = 1;
    printf("Initiating KTP socket\n");
    // Get keys
    int key_shmid_sock_info, key_shmid_SM, key_sem1, key_sem2, key_sem_SM, key_sem_sock_info;
    key_shmid_sock_info = ftok("/", 'A');
    key_shmid_SM = ftok("/", 'B');
    key_sem1 = ftok("/", 'C');
    key_sem2 = ftok("/", 'D');
    key_sem_SM = ftok("/", 'E');
    key_sem_sock_info = ftok("/", 'F');

    printf("Getting shared memory and semaphores\n");

    // create shared memory and semaphores
    shmid_sock_info = shmget(key_shmid_sock_info, sizeof(SOCK_INFO), 0666 | IPC_CREAT);
    shmid_SM = shmget(key_shmid_SM, sizeof(struct SM_entry) * N, 0666 | IPC_CREAT);
    sem1 = semget(key_sem1, 1, 0666 | IPC_CREAT);
    sem2 = semget(key_sem2, 1, 0666 | IPC_CREAT);
    sem_SM = semget(key_sem_SM, 1, 0666 | IPC_CREAT);
    sem_sock_info = semget(key_sem_sock_info, 1, 0666 | IPC_CREAT);
    printf("Checking for errors\n");
     
    // attach shared memory
    sock_info = (SOCK_INFO *)shmat(shmid_sock_info, 0, 0);
    SM = (struct SM_entry *)shmat(shmid_SM, 0, 0);

    printf("Checking for errors\n");
    fflush(stdout);

    // initialising sock_info
    sock_info->sock_id = 0;
    sock_info->err_no = 0;
    sock_info->ip_address[0] = '\0';
    sock_info->port = 0;

    printf("Initialising SM\n"); fflush(stdout);
    // initialising SM
    for (int i = 0; i < N; i++)
        SM[i].is_free = 1;

    printf("Initialising semaphores\n"); fflush(stdout);
    // initialising semaphores
    semctl(sem1, 0, SETVAL, 0);
    semctl(sem2, 0, SETVAL, 0);
    semctl(sem_SM, 0, SETVAL, 1);
    semctl(sem_sock_info, 0, SETVAL, 1);

    printf("Creating threads\n");

    // create threads S, R, etc.
    pthread_t S_thread, R_thread, G_thread;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&S_thread, &attr, S, NULL);
    pthread_create(&R_thread, &attr, R, NULL);
    pthread_create(&G_thread, &attr, G, NULL);

    printf("Threads created\n");
    fflush(stdout);

    // while loop
    while (1)
    {
        P(sem1);
        P(sem_sock_info);
        if (sock_info->sock_id == 0 && sock_info->ip_address[0] == '\0' && sock_info->port == 0)
        {
            printf("Creating socket\n");
            int sock_id = socket(AF_INET, SOCK_DGRAM, 0);
            if (sock_id == -1)
            {
                sock_info->sock_id = -1;
                sock_info->err_no = errno;
            }
            else
            {
                sock_info->sock_id = sock_id;
            }
        }
        else
        {
            printf("Binding socket\n");
            struct sockaddr_in serv_addr;
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_port = htons(sock_info->port);
            inet_aton(sock_info->ip_address, &serv_addr.sin_addr);
            if (bind(sock_info->sock_id, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
            {
                sock_info->sock_id = -1;
                sock_info->err_no = errno;
            }
        }
        V(sem_sock_info);
        V(sem2);
    }

    return 0;
}