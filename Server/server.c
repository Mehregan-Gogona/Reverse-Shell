#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#define MAX_INPUT 1024

// Function to print the current working directory
void print_pwd()
{
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
        printf("Current Working Directory: %s\n", cwd);
    }
    else
    {
        perror("getcwd() error");
    }
}

// Function to echo text to the output
void echo_text(const char *args)
{
    printf("%s\n", args);
}

// Function to change directory
void change_directory(const char *path)
{
    if (chdir(path) == 0)
    {
        print_pwd();
    }
    else
    {
        perror("cd error");
    }
}

int main()
{
    char input[MAX_INPUT];

    printf("Simple Command Server. Type 'exit' to quit.\n");
    while (1)
    {
        printf("> ");
        if (fgets(input, MAX_INPUT, stdin) == NULL)
        {
            break;
        }
        // Remove the newline character at the end
        input[strcspn(input, "\n")] = '\0';

        // Check for exit command
        if (strcmp(input, "exit") == 0)
        {
            break;
        }

        // Split the input into command and arguments
        char *cmd = strtok(input, " ");
        char *args = strtok(NULL, "\n"); // The rest is considered as the argument

        if (cmd == NULL)
        {
            continue;
        }

        // The server supports alternate command names for similar functionality
        if (strcmp(cmd, "pwd") == 0 || strcmp(cmd, "directory") == 0 ||
            strcmp(cmd, "working") == 0 || strcmp(cmd, "current") == 0)
        {
            print_pwd();
        }
        else if (strcmp(cmd, "echo") == 0)
        {
            if (args)
            {
                echo_text(args);
            }
            else
            {
                printf("\n");
            }
        }
        else if (strcmp(cmd, "ls") == 0)
        {
            // If an argument is provided, assume it's a directory to list
            char command[256];
            if (args)
            {
                snprintf(command, sizeof(command), "ls %s", args);
            }
            else
            {
                snprintf(command, sizeof(command), "ls");
            }
            system(command);
        }
        else if (strcmp(cmd, "cd") == 0)
        {
            if (args)
            {
                change_directory(args);
            }
            else
            {
                printf("cd requires a directory argument.\n");
            }
        }
        else if (strcmp(cmd, "cat") == 0)
        {
            if (args)
            {
                char command[256];
                snprintf(command, sizeof(command), "cat %s", args);
                system(command);
            }
            else
            {
                printf("cat requires a filename argument.\n");
            }
        }
        else if (strcmp(cmd, "mv") == 0)
        {
            // Expected format: mv source target
            char *src = strtok(args, " ");
            char *dest = strtok(NULL, " ");
            if (src && dest)
            {
                if (rename(src, dest) != 0)
                {
                    perror("mv error");
                }
            }
            else
            {
                printf("Usage: mv <source> <destination>\n");
            }
        }
        else if (strcmp(cmd, "cp") == 0)
        {
            // Here we rely on calling the underlying system command
            if (args)
            {
                char command[256];
                snprintf(command, sizeof(command), "cp %s", args);
                system(command);
            }
            else
            {
                printf("cp requires source and destination arguments.\n");
            }
        }
        else if (strcmp(cmd, "rm") == 0)
        {
            // Supports basic removal; note that dangerous flags like -rf are passed along as-is
            if (args)
            {
                char command[256];
                snprintf(command, sizeof(command), "rm %s", args);
                system(command);
            }
            else
            {
                printf("rm requires arguments (e.g., -rf filename or directory).\n");
            }
        }
        else if (strcmp(cmd, "mkdir") == 0)
        {
            if (args)
            {
                char command[256];
                snprintf(command, sizeof(command), "mkdir %s", args);
                system(command);
            }
            else
            {
                printf("mkdir requires a directory name.\n");
            }
        }
        else if (strcmp(cmd, "locate") == 0)
        {
            if (args)
            {
                char command[256];
                snprintf(command, sizeof(command), "locate %s", args);
                system(command);
            }
            else
            {
                printf("locate requires a search term.\n");
            }
        }
        else if (strcmp(cmd, "wheris") == 0)
        {
            if (args)
            {
                char command[256];
                snprintf(command, sizeof(command), "whereis %s", args);
                system(command);
            }
            else
            {
                printf("wheris requires a binary name.\n");
            }
        }
        else if (strcmp(cmd, "find") == 0)
        {
            if (args)
            {
                char command[256];
                snprintf(command, sizeof(command), "find %s", args);
                system(command);
            }
            else
            {
                printf("find requires arguments.\n");
            }
        }
        else if (strcmp(cmd, "grep") == 0)
        {
            if (args)
            {
                char command[256];
                snprintf(command, sizeof(command), "grep %s", args);
                system(command);
            }
            else
            {
                printf("grep requires a pattern and file arguments.\n");
            }
        }
        else if (strcmp(cmd, "sed") == 0)
        {
            if (args)
            {
                char command[256];
                snprintf(command, sizeof(command), "sed %s", args);
                system(command);
            }
            else
            {
                printf("sed requires a command and file argument.\n");
            }
        }
        else if (strcmp(cmd, "mkfs") == 0 || strcmp(cmd, "fdisk") == 0)
        {
            // Formatting and partitioning commands are typically not handled by such a server.
            printf("The command '%s' is not supported by this server for safety reasons.\n", cmd);
        }
        else if (strcmp(cmd, "nano") == 0 || strcmp(cmd, "vi") == 0 || strcmp(cmd, "emacs") == 0)
        {
            if (args)
            {
                char command[256];
                snprintf(command, sizeof(command), "%s %s", cmd, args);
                system(command);
            }
            else
            {
                printf("Usage: %s <filename>\n", cmd);
            }
        }
        else
        {
            printf("Unknown command: %s\n", cmd);
        }
    }

    return 0;
}