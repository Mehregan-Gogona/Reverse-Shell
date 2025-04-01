#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 3000        // Port number for the server
#define BUFFER_SIZE 1024 // Buffer size for data transfer

int main(int argc, char *argv[])
{
    int sock;
    struct sockaddr_in server_addr;
    char command[BUFFER_SIZE]; // Command buffer
    char buffer[BUFFER_SIZE];  // Buffer for receiving data
    int chunk_size;            // Size of the chunk received

    // Check for correct number of arguments
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <server_ip>\n", argv[0]);
        exit(EXIT_FAILURE); // Print failure message and exit
    }

    // Create and connect socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket creation error");
        exit(EXIT_FAILURE);
    }

    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    // Convert IPv4 addresses from text to binary form
    if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) <= 0)
    {
        fprintf(stderr, "Invalid address\n");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Connection Failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("Connected to server %s:%d\n", argv[1], PORT); // Print success message
    printf("Enter commands to be executed on the server. Type 'exit' to close.\n");

    while (1)
    {
        printf("127.0.0.1 $ ");
        fflush(stdout);                                     // Flush stdout to ensure prompt appears without delay
        if (fgets(command, sizeof(command), stdin) == NULL) // Read command from stdin
        {
            perror("fgets failed");
            break;
        }

        // Remove newline character from command
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
            // First read the chunk size from the server
            if (recv(sock, &chunk_size, sizeof(int), 0) <= 0)
            {
                perror("receive failed");
                exit(EXIT_FAILURE);
            }

            // Convert from network byte order to host byte order
            chunk_size = ntohl(chunk_size);

            // If chunk_size is 0, we have received all of the data
            if (chunk_size == 0)
            {
                break;
            }

            // Read the chunk data
            int bytes_received = 0;
            while (bytes_received < chunk_size)
            {
                // Calculate the remaining bytes to receive
                int remaining = chunk_size - bytes_received;

                // Calculate the amount of bytes to receive
                int to_receive = remaining < BUFFER_SIZE ? remaining : BUFFER_SIZE;

                // Receive the data from the server
                int n = recv(sock, buffer, to_receive, 0);
                if (n <= 0)
                {
                    perror("recv failed or connection closed");
                    exit(EXIT_FAILURE);
                }

                // Print chunk data directly
                fwrite(buffer, 1, n, stdout);

                // Update the number of bytes received
                bytes_received += n;
            }
        }
    }

    close(sock);
    return 0;
}
