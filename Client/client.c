#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 3000
#define BUFFER_SIZE 1024

int main(int argc, char *argv[])
{
    int sock;
    struct sockaddr_in server_addr;
    char command[BUFFER_SIZE];
    char buffer[BUFFER_SIZE];
    int chunk_size;

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <server_ip>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Create and connect socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket creation error");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) <= 0)
    {
        fprintf(stderr, "Invalid address / Address not supported\n");
        close(sock);
        exit(EXIT_FAILURE);
    }

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Connection Failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("Connected to server %s:%d\n", argv[1], PORT);
    printf("Enter commands to be executed on the server. Type 'exit' to close.\n");

    while (1)
    {
        printf("client> ");
        fflush(stdout);
        if (fgets(command, sizeof(command), stdin) == NULL)
        {
            break;
        }
        command[strcspn(command, "\n")] = '\0';

        // Send command
        if (send(sock, command, strlen(command), 0) < 0)
        {
            perror("send failed");
            break;
        }

        // If exit command, break out of loop
        if (strncmp(command, "exit", 4) == 0)
        {
            break;
        }

        // Read response in chunks
        while (1)
        {
            // First read the chunk size (4 bytes integer)
            if (recv(sock, &chunk_size, sizeof(int), 0) <= 0)
            {
                perror("recv failed or connection closed");
                exit(EXIT_FAILURE);
            }

            // Convert from network byte order
            chunk_size = ntohl(chunk_size);

            // If chunk_size is 0, we've received all chunks
            if (chunk_size == 0)
            {
                break;
            }

            // Read the chunk data
            int bytes_received = 0;
            while (bytes_received < chunk_size)
            {
                int remaining = chunk_size - bytes_received;
                int to_receive = remaining < BUFFER_SIZE ? remaining : BUFFER_SIZE;

                int n = recv(sock, buffer, to_receive, 0);
                if (n <= 0)
                {
                    perror("recv failed or connection closed");
                    exit(EXIT_FAILURE);
                }

                // Print chunk data directly
                fwrite(buffer, 1, n, stdout);
                bytes_received += n;
            }
        }
    }

    close(sock);
    return 0;
}