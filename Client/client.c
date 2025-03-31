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
    // char response[BUFFER_SIZE * 2];
    ssize_t num_received;
    // Replace the fixed-size response buffer with a dynamic one:

    char *response = NULL;
    size_t response_size = BUFFER_SIZE;
    size_t total_read = 0;

    response = malloc(response_size);
    if (!response)
    {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }

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
        // Check if buffer needs expansion
        if (total_read + BUFFER_SIZE >= response_size)
        {
            response_size *= 2;
            char *new_response = realloc(response, response_size);
            if (!new_response)
            {
                perror("realloc failed");
                free(response);
                exit(EXIT_FAILURE);
            }
            response = new_response;
        }

        ssize_t num_received = recv(sock, response + total_read, response_size - total_read - 1, 0);
        if (num_received <= 0)
        {
            perror("recv failed or connection closed");
            free(response);
            exit(EXIT_FAILURE);
        }

        total_read += num_received;
        response[total_read] = '\0';

        if (contains_FINISHER(response))
        {
            char *delim_ptr = strstr(response, FINISHER);
            *delim_ptr = '\0';
            printf("%s", response);
            break;
        }
    }

    free(response);

    close(sock);
    return 0;
}