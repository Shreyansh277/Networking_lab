#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define PORT 20000
#define BUFFER_SIZE 100
#define SERVER_IP "127.0.0.1"

int main()
{
    int sock;                  // socket file descriptor
    struct sockaddr_in server; // server address
    char buffer[BUFFER_SIZE];  // buffer for communication
    char key[27];              // key for encryption 26 alphabets + 1 for null character
    char filename[100];        // filename of which we want to retrieve the encrypted file

    while (1)
    {
        // create tcp socket
        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            perror("Cannot create socket\n");
            exit(EXIT_FAILURE);
        }

        // set server address
        server.sin_family = AF_INET;                   // internet family
        server.sin_addr.s_addr = inet_addr(SERVER_IP); // server ip address
        server.sin_port = htons(PORT);                 // port number

        // connect to server
        if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0)
        {
            perror("Cannot connect to server\n");
            exit(EXIT_FAILURE);
        }

        printf("Connected to server...\n");

        // takiing filename as input
        printf("Enter filename : ");
        scanf("%s", filename);

        FILE *f = fopen(filename, "r");
        if (f == NULL)
        {
            printf("Error : File not found\n");
            continue;
        }

        // taking key of encryption ans input
        printf("Enter key : ");
        scanf("%s", key);
        // printf("Key : %s,%ld\n",key,strlen(key));

        if (strlen(key) != 26)
        {
            printf("Error : Key should be of 26 characters\n");
            fclose(f);
            continue;
        }

        // convert key to uppercase letter
        for (int i = 0; key[i] != '\0'; i++)
        {
            if (key[i] >= 'a' && key[i] <= 'z')
            {
                key[i] -= 32;
            }
        }

        // send key to server
        send(sock, key, strlen(key), 0);

        // send file to server
        while (fgets(buffer, BUFFER_SIZE, f) != NULL)
        {
            printf("Sending : %s\n", buffer);
            send(sock, buffer, strlen(buffer), 0);
        }

        // file closed
        fclose(f);

        // signal to stop receiving file
        send(sock, "$", 1, 0);

        printf("File sent. waiting for the encryption...\n");

        // receive encrypted file from server
        f = fopen("input.txt.enc", "w");
        while (1)
        {
            int n = recv(sock, buffer, BUFFER_SIZE, 0);
            if(n<=0)break;
            buffer[n] = '\0';
            if(buffer[n-1]=='$')
            {
                buffer[n-1] = '\0';
                fprintf(f, "%s", buffer); // Write exactly the received number of bytes
                break;
            }
            fprintf(f, "%s", buffer);
        }

        printf("File received ans saved as %s.enc\n", filename);
        fclose(f);

        // ask for more files
        char choice[10];
        printf("Do you want to send more files (yes/no) : ");
        scanf("%s", choice);

        if (strcasecmp(choice, "no") == 0)
        {
            break;
        }

        close(sock);
    }
    // close connection
    close(sock);
    printf("Connection closed\n");
    return 0;
}
