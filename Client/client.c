#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 3000
#define BUFFER_SIZE 1024

// The client connects to the reverse shell server and sends commands
// which are executed by the server. The output returned from the server
// is then displayed locally.
int main(int argc, char *argv[])
{
    int sock;
    struct sockaddr_in server_addr;
    char command[BUFFER_SIZE];
    char response[BUFFER_SIZE * 2]; // Buffer for response from server
    ssize_t num_received;

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <server_ip>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket creation error");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    // Convert IPv4 address from text to binary form
    if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) <= 0)
    {
        fprintf(stderr, "Invalid address/ Address not supported\n");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // Connect to the reverse shell server
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
        command[strcspn(command, "\n")] = '\0'; // Remove newline

        // Send command to the server
        if (send(sock, command, strlen(command), 0) < 0)
        {
            perror("send failed");
            break;
        }

        // If command is "exit", break out after sending
        if (strncmp(command, "exit", 4) == 0)
        {
            break;
        }

        // Receive response from the server
        memset(response, 0, sizeof(response));
        num_received = recv(sock, response, sizeof(response) - 1, 0);
        if (num_received <= 0)
        {
            perror("recv failed or connection closed");
            break;
        }
        response[num_received] = '\0';
        printf("%s", response);
    }

    close(sock);
    return 0;
}