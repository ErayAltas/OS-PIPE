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
    int i = 0;
    char **commands;

    char *param = strtok(str, " ");
    commands = malloc(10 * sizeof(char *));
    for (int i = 0; i < 10; i++)
    {
        commands[i] = malloc(50 * sizeof(char));
    }

    while (param != NULL)
    {
        *(commands + i) = param;
        i++;
        param = strtok(NULL, " ");
    }

    return commands;
}

void *createFile(char *args)
{
    pthread_mutex_lock(&lock);

    int count = getActiveFileCount();
    printf("count : %d\n", count);

    if (count < 10)
    {
        struct arg_struct *arg_struct = args; // get args
        char *filename = arg_struct->arg1;

        printf("filename : %s\n", filename);

        int idx = -1; // int value to check file exists or not
        for (int i = 0; i < file_count; i++)
        {
            if (file_list[i] != NULL)
            {
                if (strcmp(file_list[i], filename) == 0) // check file exists or not
                {
                    idx = i;
                }
            }
        }
        if (idx == -1) // if value == -1 then file not exists, create new file
        {
            for (int i = 0; i < 10; i++)
            {
                if (file_list[i][0] == '\0')
                {
                    strcpy(file_list[i], filename);    // add filename to the list
                    FILE *file = fopen(filename, "w"); // open file

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

    pthread_mutex_unlock(&lock);
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
        file_list[idx][0] = '\0';          // remove from file list
        remove(filename);                  // remove from system
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
    char *data = arg_struct->arg2;

    printf("filename : %s\n", filename);
    printf("data : %s\n", data);

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
        fprintf(file, "%s\n", data);        // write to file

        fclose(file);

        strcpy(response, "File wr!"); // fill the response
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

        if (content[i - 1] == '\n') // delete \n for content
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
            printf("Client has been logged out.\n");
            strcpy(response, "Program has finished\n");
            responseValue = 1;
            count--;
            if (count == 0)
            {
                fd = open(named_pipe, O_WRONLY);
                write(fd, response, sizeof(response));
                close(fd);
                exit(0);
            }
        }
        // join threads
        for (int i = 0; i < 4; i++)
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
