#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#define PORT 3000
#define BUFFER_SIZE 2048

// Function to execute a command and send back its output
void process_client(int client_sock) {
    char command[BUFFER_SIZE];
    char output[BUFFER_SIZE];
    ssize_t n;
    
    // Print connection established message
    const char *welcome = "Connected to server. Type commands (or type 'exit' to disconnect).\n";
    send(client_sock, welcome, strlen(welcome), 0);

    while (1) {
        memset(command, 0, BUFFER_SIZE);
        n = recv(client_sock, command, BUFFER_SIZE - 1, 0);
        if (n <= 0) break;
        
        // Remove any trailing newline characters
        command[strcspn(command, "\n")] = '\0';
        
        // Exit if command is "exit"
        if (strcmp(command, "exit") == 0) break;
        
        FILE *fp = popen(command, "r");
        if (fp == NULL) {
            snprintf(output, BUFFER_SIZE, "Error executing command: %s\n", command);
        } else {
            memset(output, 0, BUFFER_SIZE);
            fread(output, 1, BUFFER_SIZE - 1, fp);
            pclose(fp);
        }
        
        // Send the output back to the client
        send(client_sock, output, strlen(output), 0);
    }
    close(client_sock);
}

int main() {
    int sockfd, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    // Create a TCP socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    
    // Bind socket to specified port
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    
    // Listen for incoming connections
    if (listen(sockfd, 5) < 0) {
        perror("listen");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    printf("Server listening on port %d...\n", PORT);
    
    // Accept and process connections
    while (1) {
        client_sock = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);
        if (client_sock < 0) {
            perror("accept");
            continue;
        }
        printf("Client connected: %s\n", inet_ntoa(client_addr.sin_addr));
        
        // Fork to handle multiple clients concurrently
        if (fork() == 0) {
            close(sockfd);
            process_client(client_sock);
            exit(0);
        }
        close(client_sock);
        
        // Optionally, wait for child processes to prevent zombies
        while (waitpid(-1, NULL, WNOHANG) > 0)
            ;
    }
    close(sockfd);
    return 0;
}