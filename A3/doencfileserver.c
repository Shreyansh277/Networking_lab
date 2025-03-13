#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define PORT 20000
#define BUFFER_SIZE 100

void encryptFile(char *text, const char *key)
{
    for (int i = 0; text[i] != '\0'; i++)
    {
        if (text[i] >= 'a' && text[i] <= 'z')
        {
            text[i] = key[text[i] - 'a'] + 32; // add 32  to convert it  to lower case letter
        }
        else if (text[i] >= 'A' && text[i] <= 'Z')
        {
            text[i] = key[text[i] - 'A'];
        }
    }
    return;
}

int main()
{
    int server_fd, client_fd;              // server and client file descriptor
    struct sockaddr_in server, client;     // server and client address
    socklen_t client_len = sizeof(client); // client address length
    char buffer[BUFFER_SIZE];              // buffer for communication
    char key[27];                          // key for encryption 26 alphabets + 1 for null character

    // create tcp socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Cannot create socket\n");
        exit(EXIT_FAILURE);
    }

    // set server address
    server.sin_family = AF_INET;         // internet family
    server.sin_addr.s_addr = INADDR_ANY; // any ip address
    server.sin_port = htons(PORT);       // port number

    // bind server address to socket
    if (bind(server_fd, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("Unable to bind local address\n");
        exit(EXIT_FAILURE);
    }

    // listen for incoming connections
    if (listen(server_fd, 5) < 0)
    {
        perror("Unable to listen\n");
        exit(EXIT_FAILURE);
    }

    // accept connection from client
    printf("Server is listining on port %d...\n", PORT);

    while (1)
    {
        // accept connection from client
        if ((client_fd = accept(server_fd, (struct sockaddr *)&client, &client_len)) < 0)
        {
            perror("Unable to accept connection\n");
            exit(EXIT_FAILURE);
        }

        // client connection accepted
        printf("Client connected...\n");

        // receive key from client
        read(client_fd, key, sizeof(key));
        key[26] = '\0';
        printf("Key received : %s\n", key);

        // Open file for writing
        FILE *f = fopen("127.0.0.1.5000.txt", "w");
        if (f == NULL)
        {
            perror("Cannot create file");
            close(client_fd);
            continue;
        }

        // Receive file data from client and write to file
        while (1)
        {
            int n = recv(client_fd, buffer, BUFFER_SIZE,0);
            if(n<=0)break;
            buffer[n] = '\0';
            if ( buffer[n-1]=='$')
            {
                buffer[n-1] = '\0';
                fwrite(buffer, 1, n-1, f); // Write exactly the received number of bytes
                break;
            }
            printf("Received : %s\n", buffer);
            fwrite(buffer, 1, n, f); // Write exactly the received number of bytes
            fflush(f);
        }

        // Ensure all data is written to disk

        fclose(f);

        // encrypt file
        f = fopen("127.0.0.1.5000.txt", "r");
        FILE *f1 = fopen("127.0.0.1.5000.txt.enc", "w");

        if (f == NULL || f1 == NULL)
        {
            perror("Cannot open file\n");
            close(client_fd);
            continue;
        }

        while (fgets(buffer, BUFFER_SIZE, f) != NULL)
        {
            encryptFile(buffer, key);
            fprintf(f1, "%s", buffer);
            fflush(f1);
        }

        fclose(f);
        fclose(f1);

        // send encrypted file to client
        f1 = fopen("127.0.0.1.5000.txt.enc", "r");
        while (fgets(buffer, BUFFER_SIZE, f1) != NULL)
        {
            send(client_fd, buffer, strlen(buffer), 0);
        }
        fclose(f1);

        // signal to stop receiving file
        send(client_fd, "$", 1, 0);

        printf("File encrypted and sent to client\n");

        close(client_fd);
    }

    return 0;
}