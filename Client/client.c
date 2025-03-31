#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 3000
#define BUFFER_SIZE 1024
#define FINISHER "***" // This must match the server's FINISHER

// Helper function to check if the FINISHER exists within a string.
int contains_FINISHER(const char *buffer)
{
    return strstr(buffer, FINISHER) != NULL;
}

int main(int argc, char *argv[])
{
    int sock;
    struct sockaddr_in server_addr;
    char command[BUFFER_SIZE];
    char response[BUFFER_SIZE * 2];
    ssize_t num_received;

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

        // Read response until the FINISHER is encountered.
        memset(response, 0, sizeof(response));
        int total_read = 0;
        while (1)
        {
            num_received = recv(sock, response + total_read, sizeof(response) - total_read - 1, 0);
            if (num_received <= 0)
            {
                perror("recv failed or connection closed");
                exit(EXIT_FAILURE);
            }
            total_read += num_received;
            response[total_read] = '\0';
            if (contains_FINISHER(response))
            {
                // Remove FINISHER before printing.
                char *delim_ptr = strstr(response, FINISHER);
                if (delim_ptr)
                {
                    *delim_ptr = '\0';
                }
                break;
            }
        }

        printf("%s", response);
    }

    close(sock);
    return 0;
}
