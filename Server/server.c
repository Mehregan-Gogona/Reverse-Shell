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

// Executes a command and sends its output in chunks to the client.
void execute_command(int client_sock, const char *command)
{
    char output[BUFFER_SIZE];
    FILE *fp = popen(command, "r");
    if (fp == NULL)
    {
        snprintf(output, BUFFER_SIZE, "Error executing command: %.2022s\n", command);
        send(client_sock, output, strlen(output), 0);
        return;
    }

    // Read and send command output in chunks until EOF.
    while (fgets(output, BUFFER_SIZE, fp) != NULL)
    {
        send(client_sock, output, strlen(output), 0);
    }
    pclose(fp);
}

void process_client(int client_sock)
{
    char command[BUFFER_SIZE];
    ssize_t n;

    const char *welcome = "Connected to server. Type commands (or type 'exit' to disconnect).\n";
    send(client_sock, welcome, strlen(welcome), 0);

    // Current working directory tracking (for persistent state)
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

        // Remove trailing newline
        command[strcspn(command, "\n")] = '\0';

        // Exit if command is "exit"
        if (strcmp(command, "exit") == 0)
            break;

        // Handle 'cd' command specially to update working directory.
        if (strncmp(command, "cd ", 3) == 0 || strcmp(command, "cd") == 0)
        {
            char *dir = command + 2;
            if (*dir == '\0')
            {
                dir = getenv("HOME");
                if (!dir)
                    dir = "/";
            }
            else
            {
                while (*dir == ' ')
                    dir++;
            }
            if (chdir(dir) == 0)
            {
                if (getcwd(cwd, sizeof(cwd)) == NULL)
                {
                    perror("getcwd");
                }
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

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    if (listen(sockfd, 5) < 0)
    {
        perror("listen");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    printf("Server listening on port %d...\n", PORT);

    while (1)
    {
        client_sock = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);
        if (client_sock < 0)
        {
            perror("accept");
            continue;
        }
        printf("Client connected: %s\n", inet_ntoa(client_addr.sin_addr));

        if (fork() == 0)
        {
            close(sockfd);
            process_client(client_sock);
            exit(0);
        }
        close(client_sock);
        while (waitpid(-1, NULL, WNOHANG) > 0)
            ;
    }
    close(sockfd);
    return 0;
}