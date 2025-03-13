#include<stdio.h> 
#include<stdlib.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<sys/types.h>
#include<string.h>
#include<unistd.h>
#include<sys/socket.h>


#define port 6000

int main()
{
    int n ,servfd;
    struct sockaddr_in servaddr;
    char message[100]="a.txt";
    char buffer[1000];


    // printf("ss"); fflush(stdout);
    // clearing the addr
    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_addr.s_addr= inet_addr("127.0.0.1");
    servaddr.sin_family= AF_INET;
    servaddr.sin_port = htons(port);

    servfd = socket(AF_INET,SOCK_DGRAM,0);

  
    
    // sending file name 
    sendto(servfd,message,sizeof(message),0,(struct sockaddr_in *)&servaddr,sizeof(servaddr));
    // printf("kk"); fflush(stdout);

    // waiting for response
    n = recvfrom(servfd,buffer,sizeof(buffer),0,NULL,NULL);
    buffer[n]='\0';

    if(strcmp(buffer,"NOTFOUND")==0)
    {
        printf("file not found\n");
        close(servfd);
        return 0;
    }

    FILE * f = fopen("output.txt","w");

    while(1)
    {
        // waiting for response
        n = recvfrom(servfd,buffer,sizeof(buffer),0,NULL,NULL);
        buffer[n]='\0';
        if(strcmp(buffer,"FINISH")==0)break;
        fprintf(f,"%s",buffer);
    }
    fclose(f);

    close(servfd);
    return 0;
}