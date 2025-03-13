#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<sys/socket.h>
#include<sys/types.h>


#define port 50000

int main()
{
    int n,sockfd;
    struct sockaddr_in seraddr;
    char message[100]="laaaa";
    char buffer[1000];


    bzero(&seraddr,sizeof(seraddr));
    seraddr.sin_family= AF_INET;
    seraddr.sin_port= htons(port);
    seraddr.sin_addr.s_addr= inet_addr("127.0.0.1");

    // creating socket 
    sockfd = socket(AF_INET,SOCK_STREAM,0);

    if(sockfd<0)
    {
        printf("error\n");
        return;
    }

    if(connect(sockfd,&seraddr,sizeof(seraddr))<0)
    {
        printf("connection error");
        return;
    }


    send(sockfd,message,sizeof(message),0);

    n = recv(sockfd,buffer,sizeof(buffer),0);

    buffer[n]='\n';

    printf("%s",buffer);

    close(sockfd);
    return;




}

