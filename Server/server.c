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

// Function to execute non-cd commands and send back its output
void execute_command(int client_sock, const char *command)
{
    char output[BUFFER_SIZE];
    memset(output, 0, BUFFER_SIZE);

    FILE *fp = popen(command, "r");
    if (fp == NULL)
    {
        // Limit the command string output with a precision modifier to avoid overflow
        snprintf(output, BUFFER_SIZE, "Error executing command: %.2022s\n", command);
    }
    else
    {
        fread(output, 1, BUFFER_SIZE - 1, fp);
        pclose(fp);
    }

    // Send the output back to the client
    send(client_sock, output, strlen(output), 0);
}

void process_client(int client_sock)
{
    char command[BUFFER_SIZE];
    ssize_t n;

    // Print connection established message
    const char *welcome = "Connected to server. Type commands (or type 'exit' to disconnect).\n";
    send(client_sock, welcome, strlen(welcome), 0);

    // Maintain a persistent working directory for the session
    // This will persist between commands if we use chdir() accordingly.
    char cwd[BUFFER_SIZE];
    if (getcwd(cwd, sizeof(cwd)) == NULL)
    {
        perror("getcwd");
        strcpy(cwd, "/");
    }

    while (1)
    {
        memset(command, 0, BUFFER_SIZE);
        n = recv(client_sock, command, BUFFER_SIZE - 1, 0);
        if (n <= 0)
            break;

        // Remove any trailing newline characters
        command[strcspn(command, "\n")] = '\0';

        // Exit if the command is "exit"
        if (strcmp(command, "exit") == 0)
            break;

        // Check if the command is a "cd" command to update the working directory.
        if (strncmp(command, "cd ", 3) == 0 || strcmp(command, "cd") == 0)
        {
            // Extract directory if provided.
            char *dir = command + 2;
            // If only "cd" is entered, default to the home directory
            if (*dir == '\0')
            {
                dir = getenv("HOME");
                if (!dir)
                    dir = "/";
            }
            else
            {
                // Remove leading spaces
                while (*dir == ' ')
                    dir++;
            }
            if (chdir(dir) == 0)
            {
                // Successfully changed directory, update cwd
                if (getcwd(cwd, sizeof(cwd)) == NULL)
                {
                    perror("getcwd");
                }
                // Inform client of success
                char success_msg[BUFFER_SIZE];
                snprintf(success_msg, BUFFER_SIZE, "Directory changed to: %s\n", cwd);
                send(client_sock, success_msg, strlen(success_msg), 0);
            }
            else
            {
                char err_msg[BUFFER_SIZE];
                snprintf(err_msg, BUFFER_SIZE, "Failed to change directory to: %s\n", dir);
                send(client_sock, err_msg, strlen(err_msg), 0);
            }
        }
        else
        {
            // Build the command with updated working directory if necessary.
            // For simplicity, we execute as is because the process's cwd is updated.
            execute_command(client_sock, command);
        }
    }
    close(client_sock);
}

int main()
{
    int sockfd, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    // Create a TCP socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Bind socket to specified port
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(sockfd, 5) < 0)
    {
        perror("listen");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    printf("Server listening on port %d...\n", PORT);

    // Accept and process connections
    while (1)
    {
        client_sock = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);
        if (client_sock < 0)
        {
            perror("accept");
            continue;
        }
        printf("Client connected: %s\n", inet_ntoa(client_addr.sin_addr));

        // Fork to handle multiple clients concurrently
        if (fork() == 0)
        {
            close(sockfd);
            process_client(client_sock);
            exit(0);
        }
        close(client_sock);

        // Clean up any finished child processes
        while (waitpid(-1, NULL, WNOHANG) > 0)
            ;
    }
    close(sockfd);
    return 0;
}