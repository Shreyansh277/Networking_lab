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
    struct sockaddr_in servaddr,clieaddr;
    char message[100]= "NOTFOUND";
    char buffer[1000];
    socklen_t len;
    len = sizeof(clieaddr);


    // clearing the addr
    bzero(&servaddr,sizeof(servaddr));
    servfd = socket(AF_INET,SOCK_DGRAM,0);
    servaddr.sin_addr.s_addr= INADDR_ANY;
    servaddr.sin_family= AF_INET;
    servaddr.sin_port = htons(port);

    bind(servfd,(struct sockaddr_in*)&servaddr,sizeof(servaddr));

    printf("server is running ...........\n");

    // waiting for message
    n = recvfrom(servfd,buffer,sizeof(buffer),0,(struct sockaddr_in *)&clieaddr,&len);
    buffer[n]='\0';
    // printf("%s",buffer); fflush(stdout);

    // opening the file 
    FILE * f = fopen(buffer,"r");
    if(f==NULL)
    {
        sendto(servfd,message,sizeof(message),0,(struct sockaddr_in *)&clieaddr,len);
        close(servfd);
        return 0;
    }

    while(fgets(buffer,1000,f))
    {
        sendto(servfd,buffer,sizeof(buffer),0,(struct sockaddr_in *)&clieaddr,len);
        if(strcmp(buffer,"FINISH")==0)break;
    }
    fclose(f);

    printf("sending compleated");
    close(servfd);

    
}