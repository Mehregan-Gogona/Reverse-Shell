#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 3000
#define BUFFER_SIZE 1024

int main()
{
    int sock;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("Socket creation failed");
        exit(1);
    }

    // Set up server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr); // Connect to localhost

    // Connect to server
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Connection failed");
        exit(1);
    }
    printf("Connected to server. Enter commands (e.g., 'ls', 'whoami'):\n");

    while (1)
    {
        // Get command from user
        printf("> ");
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = 0; // Remove newline

        // Send command to server
        send(sock, buffer, strlen(buffer), 0);

        // Receive and display output
        while ((recv(sock, buffer, BUFFER_SIZE - 1, 0)) > 0)
        {
            buffer[BUFFER_SIZE - 1] = '\0';
            printf("%s", buffer);
            if (strlen(buffer) < BUFFER_SIZE - 1)
                break; // End of response
        }
    }

    close(sock);
    return 0;
}