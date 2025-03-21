#include<stdio.h>
#include"ksocket.h"
#include<unistd.h>
#include<fcntl.h>

int main(int argc, char *argv[]){
    if(argc!=5){
        printf("Usage: %s <src_IP> <src_port> <dest_IP> <dest_port>\n", argv[0]);
        return 1;
    }
    int sockfd;
    struct sockaddr_in serverAddr;
    char buffer[1024];
    socklen_t addr_size;
    
    if((sockfd = k_socket(AF_INET, SOCK_KTP, 0))<0){
        perror("Error in socket creation\n");
        return 1;
    }
    
    char src_ip[16];
    strcpy(src_ip, argv[1]);
    char dest_ip[16];
    strcpy(dest_ip, argv[3]);
    uint16_t src_port=atoi(argv[2]);
    uint16_t dest_port=atoi(argv[4]);

    if(k_bind(src_ip, src_port, dest_ip, dest_port)<0){
        perror("Error in binding\n");
        return 1;
    }

    char filename[100];
    sprintf(filename, "new_%d.txt", src_port);
    int fd=open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if(fd<0){
        perror("Error in opening/creating file\n");
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(dest_port);
    serverAddr.sin_addr.s_addr = inet_addr(dest_ip);
    printf("Reading file\n");
    int recvlen;

    while(1){
        while((recvlen = k_recvfrom(sockfd, buffer, 512, 0, (struct sockaddr*)&serverAddr, &addr_size))<=0){
            printf("waiting for msg\n");
            if (recvlen <= 0) {
                sleep(1);
                continue;
            }
        }
        if(buffer[0]=='1'){
            printf("End of file\n");
            break;
        }
        printf("Writing %d bytes\n", recvlen-1);
        if(write(fd, buffer+1, recvlen-1)<0){
            perror("Error in writing to file\n");
            return 1;
        }
    }

    close(fd);
    sleep((unsigned int)(300*p));      // to make sure final ack is received by sender

    if(k_close(sockfd)<0){
        perror("Error in closing\n");
        return 1;
    }

    return 0;
}