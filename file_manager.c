#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
typedef struct Task
{
    char *command;
    char *file_name;
    char *data;
    int parameter_count;
    int client_id;
    char *message;
} Task;

void create_thread(char *);
void *start_Thread();
void submitTask(Task);
void executeTask(Task *);
void *clientConnection();
void listenAll();
void sendIdToClient();
void listenClient(int, char *);
void *clientThreadStart();
void responseCLient(int, char *);
void createTask(char *, int, char *);
void tokenize(char *, char[3][128], int *);
int dataCompareResult(char *);
void selectCommand(int, Task *);
int checkFile(char *);
void createFile(char *);
void createCommand(Task *);
void deleteCommand(Task *);
int findFile(char *);
void shiftFilesFromIndex(int);
void remove_new_line(char *);
void readCommand(Task *);
void readFile(char *, char *);
void writeCommand(Task *);
void writeFile(char *, char *);
void exitCommand(Task *);

pthread_mutex_t mutexQueue;
pthread_cond_t condQueue;
pthread_cond_t messageWait;
pthread_mutex_t mutexMessage;

pthread_t threadList[5];
pthread_t serverThread[128];
int serverThreadCount = 0;

char fileList[10][128];
int fileCount = 0;

Task taskQueue[128];
int taskCount = 0;

int id = 0;

char path[] = "/tmp/file_";

int main()
{
    pthread_mutex_init(&mutexQueue, NULL);
    pthread_mutex_init(&mutexMessage, NULL);

    pthread_cond_init(&condQueue, NULL);
    pthread_cond_init(&messageWait, NULL);

    char str[128];
    srand(time(NULL));

    create_thread(str); // create task executer threads and connection thread

    pthread_mutex_destroy(&mutexQueue);
    pthread_cond_destroy(&condQueue);

    return 0;
}
void create_thread(char *str) // create task executer threads and connection thread
{

    for (size_t i = 0; i < 5; i++)
    {
        if (i == 4)
        {
            if (pthread_create(threadList + i, NULL, clientConnection, NULL) != 0)
            {
                printf("create error");
                return;
            }
        }
        else
        {
            if (pthread_create(threadList + i, NULL, start_Thread, NULL) != 0)
            {
                printf("create error");
                return;
            }
        }
    }
    for (size_t j = 0; j < 5; j++)
    {
        if (pthread_join(threadList[j], NULL) != 0)
        {
            printf("join error");
            return;
        }
    }
}

void executeTask(Task *task)
{
    int selection = dataCompareResult(task->command);
    selectCommand(selection, task);

    free(task->command);
    free(task->data);
    free(task->file_name);
}
int dataCompareResult(char *str) // distinguishing input
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
void selectCommand(int selection, Task *task) // execute distinguished selection
{
    pthread_mutex_lock(&mutexQueue);
    switch (selection)
    {
    case 1:
        createCommand(task);
        break;
    case 2:
        deleteCommand(task);
        break;
    case 3:
        readCommand(task);
        break;
    case 4:
        writeCommand(task);
        break;
    case 5:
        exitCommand(task);
        break;
    default:
        printf("This command is unable to be executed\n");

        char message[128] = "This command is unable to be executed message \n";
        strcpy(task->message, message);
        pthread_cond_signal(&messageWait);

        break;
    }
    pthread_mutex_unlock(&mutexQueue);
}
void createCommand(Task *task) // create a file
{
    char message[128];
    memset(message, 0, 128);
    strcat(task->file_name, ".txt");
    if (checkFile(task->file_name)) // check the file is existed or capacity is exceeded
    {
        strcpy(task->message, "\nfile is already existed\n");
        pthread_cond_signal(&messageWait);
        return;
    }
    else if (fileCount == 10)
    {
        strcpy(task->message, "\ncapacity is exceeded\n");
        pthread_cond_signal(&messageWait);

        return;
    }
    else if (task->parameter_count != 2)
    {
        strcpy(task->message, "\parameter amount should be 2\n");
        pthread_cond_signal(&messageWait);

        return;
    }
    createFile(task->file_name);
    strcpy(task->message, "\nfile is created\n");
    pthread_cond_signal(&messageWait);

    printf("\nfile is created\n");
}
int checkFile(char *file_name) // check the file is existed or capacity is exceeded
{

    for (size_t i = 0; i < 10; i++)
    {
        if (strcmp(fileList[i], file_name) == 0)
        {
            return 1;
        }
    }
    return 0;
}
void createFile(char *file_name)
{
    strcpy(fileList[fileCount], file_name);
    fileCount++;

    FILE *file = fopen(file_name, "w");
    fclose(file);
}
void deleteCommand(Task *task)
{
    char message[128];
    memset(message, 0, 128);
    strcat(task->file_name, ".txt");

    if (checkFile(task->file_name) == 0) // check the file is existed or capacity is exceeded
    {
        printf("\nThis file does not exists, file name: %s\n", task->file_name);
        strcpy(task->message, "\nThis file does not exists\n");
        pthread_cond_signal(&messageWait);

        return;
    }
    else if (task->parameter_count != 2)
    {
        strcpy(task->message, "\nparameter amount should be 2\n");
        pthread_cond_signal(&messageWait);

        printf("parametre amount should be 2");
        return;
    }
    deleteFile(task->file_name);
    strcpy(task->message, "\nfile is deleted\n");
    pthread_cond_signal(&messageWait);

    printf("\nfile is deleted\n");
}

void deleteFile(char *file_name) // delete the file that its existence is known
{

    int file_index = findFile(file_name);
    shiftFilesFromIndex(file_index);
    fileCount--;

    if (remove(file_name) == 0)
    {
        printf("\nfile is successfully deleted");
    }
    else
    {
        printf("\nfile is not deleted because of an error");
    }
}
int findFile(char *file_name)
{
    for (size_t i = 0; i < 10; i++)
    {
        if (strcmp(fileList[i], file_name) == 0)
        {
            return i;
        }
    }
    return -1;
}
void shiftFilesFromIndex(int index)
{
    for (int i = index; i < 10; i++)
    {
        strcpy(fileList[i], fileList[i + 1]);
    }
}
void readCommand(Task *task)
{
    char message[128];
    memset(message, 0, 128);
    strcat(task->file_name, ".txt");

    if (checkFile(task->file_name) == 0) // check the file is existed or capacity is exceeded
    {
        printf("\nThis file does not exists, file name: %s\n", task->file_name);
        strcpy(task->message, "\nThis file does not exists\n");
        pthread_cond_signal(&messageWait);

        return;
    }
    else if (task->parameter_count != 2)
    {
        printf("parameter amount should be 2");
        strcpy(task->message, "\nparameter amount should be 2\n");
        pthread_cond_signal(&messageWait);

        return;
    }
    readFile(task->file_name, message);
    printf("\nfile is read\n");

    if (strcmp(message, "") == 0) // empty file check
    {
        strcpy(task->message, "Empty file");
    }
    else
    {
        strcpy(task->message, message);
    }
    pthread_cond_signal(&messageWait);
}
void readFile(char *file_name, char *message)
{
    FILE *file;

    file = fopen(file_name, "r");
    fread(message, 128, 1, file);
    fclose(file);
    printf("\nmessage is readed: %s\n", message);
}
void writeCommand(Task *task)
{
    char message[128];
    memset(message, 0, 128);
    strcat(task->file_name, ".txt");

    if (checkFile(task->file_name) == 0) // check the file is existed or capacity is exceeded
    {
        printf("\nThis file does not exists, file name: %s\n", task->file_name);
        strcpy(task->message, "\nThis file does not exists\n");
        pthread_cond_signal(&messageWait);

        return;
    }
    else if (task->parameter_count < 3)
    {
        printf("parameter amount should be 3 at least");
        strcpy(task->message, "\nparameter amount should be 3 at least\n");
        pthread_cond_signal(&messageWait);

        return;
    }
    strcpy(message, "command is fully successful");
    writeFile(task->file_name, task->data);
    printf("\nfile is written\n");
    strcpy(task->message, message);
    pthread_cond_signal(&messageWait);
}
void writeFile(char *file_name, char *message)
{
    FILE *file;

    printf("\nIt is written: %s\n", message);

    file = fopen(file_name, "a+");
    fprintf(file, "%s", message);
    fclose(file);
}
void exitCommand(Task *task)
{
    char message[128] = "";
    strcpy(task->message, "Connection terminated.");
    pthread_cond_signal(&messageWait);
}
void submitTask(Task task)
{

    pthread_mutex_lock(&mutexQueue);
    taskQueue[taskCount] = task;
    taskCount++;
    pthread_mutex_unlock(&mutexQueue);
    pthread_cond_broadcast(&condQueue);
}

void *start_Thread(void *smt)
{
    while (1)
    {
        Task task;

        pthread_mutex_lock(&mutexQueue);
        while (taskCount == 0)
        {
            pthread_cond_wait(&condQueue, &mutexQueue);
        }
        task = taskQueue[0];

        int i;
        for (i = 0; i < taskCount - 1; i++)
        {
            taskQueue[i] = taskQueue[i + 1];
        }
        taskCount--;
        pthread_mutex_unlock(&mutexQueue);

        executeTask(&task);
    }
}

void *clientConnection() // client try to connect
{
    while (1)
    {
        listenAll();          // manager wait for any client
        sendIdToClient();     // when client request connection, manager assign id
        createServerThread(); // thread for new client
    }
}
void listenAll() // manager wait for any client
{

    char oku[128];
    memset(oku, 0, 128);

    int fd;
    char *myfifo = "/tmp/file_manager";

    mkfifo(myfifo, 0666);

    fd = open(myfifo, O_RDONLY);
    read(fd, oku, 128); // blocking
    printf("client reader: %s\n", oku);

    close(fd);
}
void sendIdToClient() // when client request connection, manager assign id
{
    char str[128];
    memset(str, 0, 128);
    sprintf(str, "%d", id);

    int fd;
    char *myfifo = "/tmp/file_manager";

    mkfifo(myfifo, 0666);

    fd = open(myfifo, O_WRONLY);
    write(fd, str, 128); // blocking
    close(fd);
}

void createServerThread() // thread for new client
{
    pthread_mutex_lock(&mutexQueue);

    serverThreadCount++;

    if (pthread_create(serverThread + serverThreadCount, NULL, &clientThreadStart, NULL) != 0)
    {
        perror("Failed to create the thread");
    }
    pthread_mutex_unlock(&mutexQueue);
}
void *clientThreadStart() // client process specify in here
{
    int thread_id = id; // client id
    id++;
    char command[128];   // client command
    char message[] = ""; // message for client

    while (1)
    {
        char *task_message = (char *)malloc(128 * sizeof(char));
        strcpy(command, "");
        memset(command, 0, 128);

        listenClient(thread_id, command); // certain client listen to in here

        createTask(command, thread_id, task_message); // create a task to send into task array.
        if (strcmp("exit\n", command) == 0)
        {
            printf("\ncommand %s entered, one of client is terminated.\n", command);

            break;
        }

        // wait until message come.
        pthread_mutex_lock(&mutexMessage);
        while (strcmp(task_message, "") == 0)
        {
            pthread_cond_wait(&messageWait, &mutexMessage);
        }
        pthread_mutex_unlock(&mutexMessage);

        responseCLient(thread_id, task_message); // respond to clinent what happened to command after executing.

        free(task_message);
    }
    responseCLient(thread_id, message);
}
void listenClient(int thread_id, char *command) // certain client is waited for respond
{
    char file_path[128] = "";
    memset(file_path, 0, 128);
    int fd;

    char str_thread_id[128];
    sprintf(str_thread_id, "%d", thread_id);

    strcat(file_path, path);
    strcat(file_path, str_thread_id);

    // mkfifo(file_path, 0666);
    fd = open(file_path, O_RDONLY);
    read(fd, command, 128); // blocking

    printf("\n------------------\n");
    printf("\nclient read: %s\n", command);
    close(fd);
}
void responseCLient(int thread_id, char *message) // send message to client for client process situation
{

    char response[128] = "";
    memset(response, 0, 128);

    int fd;

    char str_thread_id[128];
    sprintf(str_thread_id, "%d", thread_id);

    strcat(response, path);
    strcat(response, str_thread_id);

    mkfifo(response, 0666);

    fd = open(response, O_WRONLY);
    write(fd, message, 128); // blocking

    close(fd);
}
void createTask(char *command, int client_id, char *message) // create a task to send into task array.
{
    char splitted_command[3][128];
    strcpy(splitted_command, "");
    memset(splitted_command, 0, 128);
    strcpy(splitted_command + 1, "");
    memset(splitted_command + 1, 0, 128);
    strcpy(splitted_command + 2, "");
    memset(splitted_command + 2, 0, 128);

    int parameter_count = 0;

    tokenize(command, splitted_command, &parameter_count);

    char *task_command = (char *)malloc(128 * sizeof(char));
    char *task_file_name = (char *)malloc(128 * sizeof(char));
    char *task_data = (char *)malloc(128 * sizeof(char));

    strcpy(task_command, splitted_command[0]);
    strcpy(task_file_name, splitted_command[1]);
    strcpy(task_data, splitted_command[2]);

    Task task = {
        .command = task_command,
        .file_name = task_file_name,
        .data = task_data,
        .parameter_count = parameter_count,
        .client_id = client_id,
        .message = message};

    submitTask(task);
}
void tokenize(char *str, char parameters[3][128], int *index)
// make your inputs seperated
{
    char *token = strtok(str, " ");

    char write_message[128] = "";
    memset(write_message, 0, 128);

    int i = 0;
    while (token != NULL)
    {

        if (i >= 2 && strcmp(parameters[0], "write") == 0)
        {
            strcat(write_message, token);
            strcat(write_message, " ");
            token = strtok(NULL, " ");
        }
        else
        {
            strcpy(parameters[i], token);
            remove_new_line(parameters[i]); // remove \n at the end if exist
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
        i++;
        remove_new_line(write_message);
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
    if ((length > 0) && (string[length - 2] == '\n'))
    {
        string[length - 2] = '\0';
    }
}
