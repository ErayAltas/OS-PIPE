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

int getIndex(char *data)
{
    int index = 0;
    for (int i = 0; data[i] != '\0'; i++)
    {
        index++;
    }
    return index;
}

int main(int argc, char *argv[])
{
    int fd;
	int isRunning = 1;
	char *myfifo = "/tmp/myfifo";

	mkfifo(myfifo, 0666);
    char response2[DATA_SIZE];
    char init[DATA_SIZE] = "init";
    fd = open(myfifo, O_WRONLY); // open pipe to write
    write(fd, init, sizeof(init)); // write to pipe
    close(fd); // close pipe

    while (isRunning){

        char input[DATA_SIZE];
        char response[DATA_SIZE];

        fgets(input, DATA_SIZE, stdin); // get input from client

		if (input[getIndex(input) - 1] == '\n'){ // check input for last index
            input[getIndex(input) - 1] = '\0';
        }

        fd = open(myfifo, O_WRONLY); // open pipe for writing
        write(fd, input, sizeof(input)); // write to pipe
		close(fd); // close pipe

        if (strcmp(input, "exit") == 0){
			isRunning = 0;
		}

		fd = open(myfifo, O_RDONLY); // open pipe to read
		read(fd, response, DATA_SIZE); // read pipe
		printf("%s\n", response); 
	}

	return 0;
}