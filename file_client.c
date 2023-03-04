#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

void menu_line();
int dataCompareResult(char *);
void send(char *);
void clientConnection();
void sendRequest();
void getResponse();
void clientCommand(char *);
void receive();
void tokenize(char *, char[3][128], int *);
void remove_new_line(char *);

int sayi = 0;
pthread_mutex_t toplama;
pthread_mutex_t kare;

int myid = -1;
char path[] = "/tmp/file_";

pthread_t tcb;

int main()
{
    char command[128];
    clientConnection();

    while (1)
    {
        menu_line();
        fgets(command, 128, stdin);

        char mynew[128];
        strcpy(mynew, command);
        char parameters[3][128];
        int count = 0;

        tokenize(mynew, parameters, &count);
        int comparison = dataCompareResult(parameters[0]);

        switch (comparison)
        {
        case 1:
        case 2:
        case 3:
        case 4:
            clientCommand(command);

            break;
        case 5:
            printf("\nProgram is closed\n");
            clientCommand(command);
            exit(0);
            break;
        default:
            printf("This command is unable to be executed\n");
            break;
        }
    }
    return 0;
}

void menu_line()
{
    printf("What is your command?\n");
    printf("-> Create a file (input should be 'command file_name')\n");
    printf("-> Delete a file (input should be 'command file_name')\n");
    printf("-> Read a file (input should be 'command file_name')\n");
    printf("-> Write inside file (input should be 'command file_name data')\n");
    printf("-> Exit\n");

    printf("Your command: ");
}
int dataCompareResult(char *str)
{
    if (strcmp(str, "create\n") == 0 || strcmp(str, "create") == 0)
    {
        return 1;
    }
    else if (strcmp(str, "delete\n") == 0 || strcmp(str, "delete") == 0)
    {
        return 2;
    }
    else if (strcmp(str, "read\n") == 0 || strcmp(str, "read") == 0)
    {
        return 3;
    }
    else if (strcmp(str, "write\n") == 0 || strcmp(str, "write") == 0)
    {
        return 4;
    }
    else if (strcmp(str, "exit\n") == 0 || strcmp(str, "exit") == 0)
    {
        return 5;
    }

    return -1;
}
void clientCommand(char *command)
{
    send(command);
    receive();
}
void send(char *str)
{
    printf("--------------------------\n");
    int fd;

    mkfifo(path, 0666);

    // printf("\ncontent client: %s\n", str);
    //  printf("path=%s\n", path);
    fd = open(path, O_WRONLY);
    write(fd, str, 128); // blocking

    close(fd);
}
void receive()
{
    char oku[128];
    int fd;
    mkfifo(path, 0666);

    fd = open(path, O_RDONLY);
    read(fd, oku, 128); // blocking
    printf("received message : %s\n", oku);

    close(fd);
}
void clientConnection()
{
    sendRequest();
    getResponse();
    // pthread_create(&tcb, NULL, receive, NULL);
}

void sendRequest()
{
    int fd;
    char *myfifo = "/tmp/file_manager";

    mkfifo(myfifo, 0666);

    fd = open(myfifo, O_WRONLY);
    write(fd, "\nnew client want to enter\n", 128); // blocking

    close(fd);
}
void getResponse()
{
    char oku[128];
    int fd;
    char *myfifo = "/tmp/file_manager";

    mkfifo(myfifo, 0666);

    fd = open(myfifo, O_RDONLY);
    read(fd, oku, 128); // blocking
    printf("Connection successful \n", oku);

    strcat(path, oku);
    // printf("path : %s\n", path);

    close(fd);
}
void tokenize(char *str, char parameters[3][128], int *index)
// make your inputs seperated
{
    char *token = strtok(str, " ");

    char write_message[128] = "";

    int i = 0;
    while (token != NULL)
    {

        if (i >= 2 && strcmp(parameters[0], "write") == 0)
        {
            strcat(write_message, token);
            strcat(write_message, " ");
            strcpy(parameters[i], token);
            token = strtok(NULL, " ");
        }
        else
        {
            strcpy(parameters[i], token);
            remove_new_line(parameters[i]);
            i++;
            token = strtok(NULL, " ");
            if (i == 3)
            {
                break;
            }
        }
    }
    if (strcmp(parameters[0], "write") == 0)
    {
        strcpy(parameters[2], write_message);
    }
    *index = i;
}
void remove_new_line(char *string)
{
    size_t length = strlen(string);
    if ((length > 0) && (string[length - 1] == '\n'))
    {
        string[length - 1] = '\0';
    }
}
