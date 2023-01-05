#include <sys/msg.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>

#define DATA_SIZE 128
#define FILE_NOT_FOUND "File not found!"
#define FILE_FULL "File list is full!"
#define FILE_CREATED "File created!"
#define FILE_ALREADY_IN_LIST "File is already in the list!"
#define FILE_DELETED "File deleted!"
#define FILE_WROTE "File wrote!"
#define FILE_READ "File read!"
#define CLIENT_IS_READY "Ready to communicate!.."
#define CLIENT_FINISHED "Client has finished!"

const int file_count = 10;
const int thread_count = 5;

pthread_t threads[thread_count];
char file_list[file_count][50];

pthread_mutex_t lock;
pthread_cond_t cond;

char response[DATA_SIZE];

int count = 0;

struct arg_struct
{
    char *arg1;
    char *arg2;
};

int getActiveFileCount()
{
    int count = 0;
    for (int i = 0; i < file_count; i++)
    {
        if (strlen(file_list[i]) > 0)
        {
            count++;
        }
    }

    return count;
}

char **tokenizeCommands(char *str)
{
    // Allocate memory for the array of strings
    char **commands = malloc(10 * sizeof(char *));
    for (int i = 0; i < 10; i++)
    {
        // Allocate memory for each individual string
        commands[i] = malloc(50 * sizeof(char));
    }

    // Initialize variables for strtok loop
    int i = 0;
    char *parameter = strtok(str, " ");

    // Tokenize the input string
    while (parameter != NULL)
    {
        // Copy the current token into the commands array
        strcpy(commands[i], parameter);
        i++;
        parameter = strtok(NULL, " ");
    }

    // Return the array of commands
    return commands;
}

void *createFile(char *args)
{
    pthread_mutex_lock(&lock); // lock mutex

    int count = getActiveFileCount(); // get active file count
    printf("count : %d\n", count);

    if (count < 10) // since the max file count will be 10, it is necessary to check.
    {
        struct arg_struct *arg_struct = args; // get args
        char *filename = arg_struct->arg1;

        printf("filename : %s\n", filename);

        int idx = -1; // int value to check file exists or not
        for (int i = 0; i < file_count; i++)
        {
            if (file_list[i] != NULL) // TODO strlen
            {
                if (strcmp(file_list[i], filename) == 0) // check file exists or not
                {
                    idx = i;
                }
            }
        }
        if (idx == -1) // if value == -1 then file not exists, create new file
        {
            for (int i = 0; i < file_count; i++)
            {
                if (file_list[i][0] == '\0')
                {
                    FILE *file = fopen(filename, "w"); // open file
                    strcpy(file_list[i], filename);    // add filename to the list

                    fclose(file);

                    strcpy(response, "File created!"); // fill the response
                    break;
                }
            }
        }
        else // file exists
        {
            strcpy(response, "File is already in the list!"); // fill the response
        }
        // print list of files
        for (int i = 0; i < file_count; i++)
        {
            printf("%d : %s\n", i, file_list[i]);
        }
    }
    else
    {
        strcpy(response, "File list is full!"); // fill the response
    }

    pthread_mutex_unlock(&lock); // unlock mutex
}

void *deleteFile(char *args)
{
    pthread_mutex_lock(&lock); // lock mutex

    struct arg_struct *arg_struct = args; // get args
    char *filename = arg_struct->arg1;

    printf("filename : %s\n", filename);

    int idx = -1;
    for (int i = 0; i < file_count; i++)
    {
        if (file_list[i] != NULL)
        {
            if (strcmp(file_list[i], filename) == 0) // check file exists or not
            {
                idx = i;
                break;
            }
        }
    }

    if (idx != -1)
    {
        file_list[idx][0] = '\0'; // remove from file list
        remove(filename);         // remove from system

        strcpy(response, "File deleted!"); // fill the response
    }
    else
    {
        strcpy(response, "File not found!"); // fill the response
    }

    pthread_mutex_unlock(&lock); // unlock the mutex
}

void *writeFile(char *args)
{
    pthread_mutex_lock(&lock); // lock mutex

    struct arg_struct *arg_struct = args; // get args
    char *filename = arg_struct->arg1;
    char *content = arg_struct->arg2;

    printf("filename : %s\n", filename);
    printf("content : %s\n", content);

    int idx = -1;

    for (int i = 0; i < 10; i++)
    {
        if (file_list[i] != NULL)
        {
            if (strcmp(file_list[i], filename) == 0) // check if the file exists
            {
                idx = i;
                break;
            }
        }
    }

    if (idx != -1) // if file not exists
    {

        FILE *file = fopen(filename, "a+"); // open file in append mode
        fprintf(file, "%s\n", content);     // write to file

        fclose(file);

        strcpy(response, "File wrote!"); // fill the response
    }
    else
    {
        strcpy(response, "File not found!"); // fill the response
    }

    pthread_mutex_unlock(&lock); // unlock mutex
}

void *readFile(char *args)
{
    pthread_mutex_lock(&lock); // lock mutex

    struct arg_struct *arg_struct = args; // get args
    char *filename = arg_struct->arg1;

    printf("filename : %s\n", filename);

    int idx = -1;

    for (int i = 0; i < file_count; i++)
    {
        if (file_list[i] != NULL)
        {
            if (strcmp(file_list[i], filename) == 0) // check if the file exists
            {
                idx = i;
                break;
            }
        }
    }

    if (idx != -1)
    {
        FILE *f = fopen(filename, "r"); // open file to read
        char ch;
        char content[128];
        int i = 0;

        while ((ch = fgetc(f)) != EOF) // read file
        {
            content[i] = ch;
            i++;
        }

        if (content[i - 1] == '\n') // delete '\n' char for content
        {
            content[i - 1] = '\0';
        }

        fclose(f);

        strcpy(response, content); // fill the response
    }
    else
    {
        strcpy(response, "File not found!"); // fill the response
    }

    pthread_mutex_unlock(&lock); // unlock the mutex
}

int main(int argc, char *argv[])
{
    // define variables
    char **commands;
    int fd;
    int responseValue = 0;
    char *named_pipe = "/tmp/file_manager_named_pipe"; // fifo for communication
    char data[DATA_SIZE];

    pthread_mutex_init(&lock, NULL); // init mutex and cond
    pthread_cond_init(&cond, NULL);

    memset(file_list, 0, file_count * 50 * sizeof(char)); // clear file list

    while (1)
    {
        fd = open(named_pipe, O_RDONLY); // open the pipe for reading
        read(fd, data, DATA_SIZE);

        commands = tokenizeCommands(data); // tokenize buf to execute commands

        struct arg_struct arg_struct;
        arg_struct.arg1 = commands[1];
        arg_struct.arg2 = commands[2];

        if (strcmp(commands[0], "init") == 0) // if first parameter is init
        {
            count++;
            printf("%d.client is initiliazed\n", count);

            strcpy(response, "Ready to communicate!.."); // fill the response
            responseValue = 1;
        }
        else if (strcmp(commands[0], "create") == 0) // if first parameter is create then call create
        {
            pthread_create(&threads[0], NULL, createFile, &arg_struct);
            responseValue = 1;
        }
        else if (strcmp(commands[0], "delete") == 0) // if first parameter is delete then call delete
        {
            pthread_create(&threads[1], NULL, deleteFile, &arg_struct);
            responseValue = 1;
        }
        else if (strcmp(commands[0], "write") == 0) // if first parameter is write then call write
        {
            pthread_create(&threads[2], NULL, writeFile, &arg_struct);
            responseValue = 1;
        }
        else if (strcmp(commands[0], "read") == 0) // if first parameter is read then call read
        {
            pthread_create(&threads[3], NULL, readFile, &arg_struct);
            responseValue = 1;
        }
        else if (strcmp(commands[0], "exit") == 0) // if first parameter is exit then call exit
        {
            printf("Client logged out!");
            strcpy(response, "Client finished!"); // fill the response
            responseValue = 1;
            count--; // decrease active client count
            if (count == 0)
            {
                fd = open(named_pipe, O_WRONLY); // open pipe and write
                write(fd, response, sizeof(response));
                close(fd);
                exit(0); // finish program
            }
        }
        // wait threads until they finish
        for (int i = 0; i < thread_count; i++)
        {
            pthread_join(threads[i], NULL);
        }
        // responseValue == 1 ? there is response, responseValue == 0 ? no response
        if (responseValue == 1)
        {
            fd = open(named_pipe, O_WRONLY);
            write(fd, response, sizeof(response));
            close(fd);
        }
    }
    // destroy mutex and cond before exiting
    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&cond);

    exit(0);
}
