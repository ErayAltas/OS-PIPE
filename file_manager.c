#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdlib.h>

#define DATA_SIZE 128

const char file_count = 10;
const char thread_count = 5;

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
    int file_count = 0;
    for (int i = 0; i < file_list; i++)
    {
        if (file_list[i] != NULL)
        {
            file_count++;
        }
    }

    return file_count;
}

void *createFile(char *args)
{
    pthread_mutex_lock(&lock);

    int count = getActiveFileCount();
    printf("count : %d\n", count);

    if (count <= 10)
    {
        struct arg_struct *arg_struct = args; // get args
        char *file_name = arg_struct->arg1;
        printf("filename : %s\n", file_name);

        int idx = -1; // int value to check file exists or not
        for (int i = 0; i < file_count; i++)
        {
            if (file_list[i] != NULL)
            {
                if (strcmp(file_list[i], file_name) == 0) // check file exists or not
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
                    strcpy(file_list[i], file_name); // add filename to the list

                    FILE *file = fopen(file_name, "w"); // open file
                    strcpy(response, "File created!");  // fill the response

                    fclose(file);

                    break;
                }
            }
        }
        else // file exists
        {
            strcpy(response, "File already exists!");
        }
    }
    else
    {
        strcpy(response, "File list is full!"); // fill the response
    }

    for (int i = 0; i < file_count; i++)
    {
        printf("%d : %s\n", i, file_list[i]);
    }

    pthread_mutex_unlock(&lock); // unlock mutex
}

void *deleteFile(char *args)
{

    pthread_mutex_lock(&lock); // lock mutex

    struct arg_struct *arg_struct = args; // get args
    char *file_name = arg_struct->arg1;
    printf("filename : %s", file_name);

    int idx = -1;

    for (int i = 0; i < file_count; i++)
    {
        if (file_list[i] != NULL)
        {
            if (strcmp(file_list[i], file_name) == 0) // check file exists or not
            {
                idx = i;
                break;
            }
        }
    }

    if (idx != -1)
    {

        file_list[idx][0] = '\0';          // remove from file list
        remove(file_name);                 // remove from system
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
    char *file_name = arg_struct->arg1;
    char *data = arg_struct->arg2;

    printf("filename : %s\n", file_name);
    printf("data : %s\n", data);

    int idx = -1;

    for (int i = 0; i < 10; i++)
    {

        if (file_list[i] != NULL)
        {
            if (strcmp(file_list[i], file_name) == 0) // check if the file exists
            {
                idx = i;
                break;
            }
        }
    }

    if (idx != -1) // if file not exists
    {

        FILE *file = fopen(file_name, "a+"); // open file in append mode
        fprintf(file, "%s\n", data);         // write to file

        fclose(file);

        strcpy(response, "File Writed!"); // fill the response
    }
    else
    {
        strcpy(response, "Yazılacak Dosya Bulunamadı"); // fill the response
    }

    pthread_mutex_unlock(&lock); // unlock mutex
}

void *readFile(char *args)
{
    pthread_mutex_lock(&lock); // lock mutex

    struct arg_struct *arg_struct = args; // get args
    char *file_name = arg_struct->arg1;
    printf("filename : %s", file_name);

    int idx = -1;

    for (int i = 0; i < file_count; i++)
    {
        if (file_list[i] != NULL)
        {
            if (strcmp(file_list[i], file_name) == 0) // check if the file exists
            {
                idx = i;
                break;
            }
        }
    }

    if (idx != -1)
    {

        FILE *fptr = fopen(file_name, "r"); // open file to read
        char c;
        while ((c = fgetc(fptr)) != EOF) // read file
        {
            printf("%c", c);
        }

        fclose(fptr);
        strcpy(response, "File read!"); // fill the response
    }
    else
    {
        strcpy(response, "File not found!"); // fill the response
    }

    pthread_mutex_unlock(&lock); // unlock the mutex
}

char **matrixGenerate(int row, int column)
{
    int i;
    char **matrix = malloc(row * sizeof(int *));
    for (i = 0; i < row; i++)
    {
        matrix[i] = malloc(column * sizeof(int));
    }

    return matrix;
}

char **tokenizeCommands(char *array)
{
    int i = 0;
    char *p = strtok(array, " ");

    char **arr;

    arr = matrixGenerate(10, 10);

    while (p != NULL)
    {
        *(arr + i) = p;
        i++;
        p = strtok(NULL, " ");
    }

    return arr;
}

int main()
{
    pthread_mutex_init(&lock, NULL); // init mutex and cond
    pthread_cond_init(&cond, NULL);

    char **commands;
    void *status;
    int fd;
    char *myfifo = "/tmp/myfifo"; // fifo
    memset(file_list, '\0', sizeof(file_list));

    while (1)
    {
        char buf[DATA_SIZE];

        fd = open(myfifo, O_RDONLY); // open the pipe for reading
        read(fd, buf, DATA_SIZE);
        close(fd);

        printf("buf : %s\n", buf);

        commands = tokenizeCommands(buf); // tokenize buf to execute commands

        struct arg_struct arg_struct;

        arg_struct.arg1 = commands[1];
        arg_struct.arg2 = commands[2];

        printf("command 1 : %s\n", commands[1]);
        printf("command 2 : %s\n", commands[2]);

        if (strcmp(commands[0], "init") == 0) // if first parameter is init
        {
            count++;
            strcpy(response, "Client initiliazed!"); // fill the response
            printf("%d\n", count);
        }
        else if (strcmp(commands[0], "create") == 0) // if first parameter is create then call create
        {
            pthread_create(&threads[0], NULL, createFile, &arg_struct);
        }
        else if (strcmp(commands[0], "delete") == 0) // if first parameter is delete then call delete
        {
            pthread_create(&threads[1], NULL, deleteFile, &arg_struct);
        }
        else if (strcmp(commands[0], "write") == 0) // if first parameter is write then call write
        {
            pthread_create(&threads[2], NULL, writeFile, &arg_struct);
        }
        else if (strcmp(commands[0], "read") == 0) // if first parameter is read then call read
        {
            pthread_create(&threads[3], NULL, readFile, &arg_struct);
        }
        else if (strcmp(commands[0], "exit") == 0) // if first parameter is exit then call exit
        {
            strcpy(response, "Client has finished!");
            count--;
            if (count == 0)
            {
                fd = open(myfifo, O_WRONLY);
                write(fd, response, sizeof(response));
                close(fd);
                exit(0);
            }
            printf("%d\n", count);
        }

        for (int i = 0; i < thread_count; i++)
        {
            pthread_join(threads[i], &status); // join threads
        }

        fd = open(myfifo, O_WRONLY);
        write(fd, response, sizeof(response));
        close(fd);
    }

    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&cond);

    exit(0);
}
