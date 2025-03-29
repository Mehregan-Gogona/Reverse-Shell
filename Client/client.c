#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"   // Change to your server's IP if needed
#define PORT 3000
#define BUFFER_SIZE 2048

int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    char command[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    ssize_t n;
    
    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    
    // Define the server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        exit(EXIT_FAILURE);
    }
    
    // Connect to server
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        exit(EXIT_FAILURE);
    }
    
    // Receive welcome message
    n = recv(sockfd, response, BUFFER_SIZE - 1, 0);
    if(n > 0) {
        response[n] = '\0';
        printf("%s", response);
    }
    
    // Loop to send commands
    while (1) {
        printf("> ");
        if (!fgets(command, BUFFER_SIZE, stdin)) break;
        // Send command to server
        send(sockfd, command, strlen(command), 0);
        
        // If command is "exit", break loop
        if (strncmp(command, "exit", 4) == 0)
            break;
        
        // Receive response from server
        memset(response, 0, BUFFER_SIZE);
        n = recv(sockfd, response, BUFFER_SIZE - 1, 0);
        if (n > 0) {
            response[n] = '\0';
            printf("%s", response);
        }
    }
    close(sockfd);
    return 0;
}