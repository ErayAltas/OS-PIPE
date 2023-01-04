#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <signal.h>

#define DATA_SIZE 128

int getLen(char *data)
{
    int len = 0;
    int i = 0;

    while (data[i] != '\0')
    {
        len++;
        i++;
    }

    return len;
}

main()
{
    int isRunning = 1;
    int fd;

    char *myfifo = "/tmp/file_manager_named_pipe";
    mkfifo(myfifo, 0666);

    char initData[DATA_SIZE] = "init";

    fd = open(myfifo, O_WRONLY);           // open pipe to write
    write(fd, initData, sizeof(initData)); // write pipe to init clients
    close(fd);                             // close pipe

    while (isRunning)
    {
        char data[DATA_SIZE];
        char response[DATA_SIZE];

        fgets(data, DATA_SIZE, stdin); // get input from client

        if (data[getLen(data) - 1] == '\n') // check input for last index
        {
            data[getLen(data) - 1] = '\0';
        }

        fd = open(myfifo, O_WRONLY);   // open pipe for writing
        write(fd, data, sizeof(data)); // write to pipe
        close(fd);

        if (strcmp(data, "exit") == 0)
        {
            isRunning = 0;
        }

        fd = open(myfifo, O_RDONLY); // open pipe to read
        read(fd, response, DATA_SIZE);     // read pipe
        printf("%s\n", response);
    }
    return 0;
}