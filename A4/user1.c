#include<stdio.h>
#include"ksocket.h"
#include<unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

int main(int argc, char *argv[]){
    if(argc!=5){
        printf("Usage: %s <src_IP> <src_port> <dest_IP> <dest_port>\n", argv[0]);
        return 1;
    }
    int sockfd;
    struct sockaddr_in serverAddr;
    char buffer[1024];
    
    if((sockfd = k_socket(AF_INET, SOCK_KTP, 0))<0){
        perror("Error in socket creation\n");
        return 1;
    }
    
    char src_ip[16];
    strcpy(src_ip, argv[1]);
    char dest_ip[16];
    strcpy(dest_ip, argv[3]);
    int src_port=atoi(argv[2]);
    int dest_port=atoi(argv[4]);

    if(k_bind(src_ip, src_port, dest_ip, dest_port)<0){
        perror("Error in binding\n");
        return 1;
    }

    int fd=open("input.txt", O_RDONLY);
    if(fd<0){
        perror("Error in opening file\n");
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(dest_port);
    inet_aton(dest_ip, &serverAddr.sin_addr);
    printf("Reading file\n");
    int readlen;
    buffer[0]='0';
    int seq=1;
    int numOfMessages=0;
    while((readlen=read(fd, buffer+1, 511))>0){
        printf("Sending %d bytes\n",readlen+1);
        int sendlen;
        while(1){
            while((sendlen=k_sendto(sockfd, buffer, readlen+1, 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr)))<0 && errno==ENOBUFS){
                sleep(1);
            }
            if(sendlen>=0)
                break;
            perror("Error in sending\n");
            return 1;
        }
        printf("Sent %d bytes, seq %d\n", sendlen, seq);
        seq=(seq+1)%256;
        numOfMessages++;
    }
    buffer[0]='1';
    int sendlen;
    while(1){
        while((sendlen=k_sendto(sockfd, buffer, 1, 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr)))<0 && errno==ENOBUFS){
            sleep(1);
        }
        if(sendlen>=0){
            numOfMessages++;
            printf("Sending Compleate. No. of messages sent = %d\n", numOfMessages);
            break;
        }
        perror("Error in sending\n");
        return 1;
    }
    sleep((unsigned int)(300*p));   // To ensure all messages are sent
    printf("File sent.\n");

    close(fd);

    if(k_close(sockfd)<0){
        perror("Error in closing\n");
        return 1;
    }

    return 0;
}