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
    int n,sockfd,newfd;
    struct sockaddr_in seraddr,cliaddr;
    char message[100]="kaaaa";
    char buffer[1000];
    int clilen;



    bzero((struct sockaddr_in*)&seraddr,sizeof(seraddr));
    seraddr.sin_family= AF_INET;
    seraddr.sin_port= htons(port);
    seraddr.sin_addr.s_addr= INADDR_ANY;

    // creating socket 
    sockfd = socket(AF_INET,SOCK_STREAM,0);

    if(sockfd<0)
    {
        printf("error\n");
        return 0;
    }

    if(bind(sockfd,(struct sockaddr_in*)&seraddr,sizeof(seraddr))<0)
    {
        printf("binding error");
        return 0;
    }


    listen(sockfd,5);

    while(1)
    {
        clilen = sizeof(cliaddr);
        newfd= accept(sockfd,(struct sockaddr_in*)&cliaddr,&clilen);

        if(newfd<0)
        {
            printf("connecting error");
            return 0;
        }


        n = recv(newfd,buffer,sizeof(buffer),0);
        buffer[n]= '\0';

        printf("%s",buffer); fflush(stdout);

        send(newfd,message,sizeof(message),0);

        close(newfd);

    }

    


return 0;

}

