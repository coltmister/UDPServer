//
// Created by COLT-ADM on 20.10.2019.
//
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

int sock, count;
pthread_mutex_t lock;
clock_t begin;
char *serverIP = "127.0.0.1";
int serverPort = 8088;
FILE *logFile;


char *currentTimestamp() {
    time_t timer;
    char *buffer = (char *) malloc(sizeof(char) * 26);
    struct tm *tm_info;
    time(&timer);
    tm_info = localtime(&timer);
    strftime(buffer, 26, "%d.%m.%Y %H:%M:%S", tm_info);
    return buffer;
}

void error(char *err) {
    if (logFile == NULL) {
        printf("%s\t%s\n", currentTimestamp(), err);
        fflush(stdin);
    } else {
        fprintf(logFile, "%s\t%s\n", currentTimestamp(), err);
        fflush(logFile);
        fclose(logFile);
    }
    perror(err);
    exit(1);
}

void quit() {
    printf("\n%s\tЗавершение работы сервера\n", currentTimestamp());
    fprintf(logFile, "\n%s\tЗавершение работы сервера\n", currentTimestamp());
    fflush(logFile);
    fflush(stdin);
    close(sock);
    fclose(logFile);
    exit(0);
}

void quitWithLog() {
    clock_t end = clock();
    double time_spent = (double) (end - begin) / CLOCKS_PER_SEC;
    fprintf(logFile, "Завершение работы сервера.\n"
                     "Сервер работал: %f с\n"
                     "Обслуженные запросы: %d\n"
                     "Текущее время: %s\n", time_spent, count, currentTimestamp());
    printf("Завершение работы сервера.\n"
           "Сервер работал: %f с\n"
           "Обслуженные запросы: %d\n"
           "Текущее время: %s\n", time_spent, count, currentTimestamp());
    fflush(logFile);
    fflush(stdin);
    fflush(stdin);
    close(sock);
    fclose(logFile);
    exit(0);
}

struct arg_struct {
    char *data;
    struct sockaddr_in clientAddress;
};

void *triangleSquare(void *arguments) {
    pthread_mutex_lock(&lock);
    double input[6];
    struct arg_struct *args = (struct arg_struct *) arguments;
    char *ptrEnd = args->data;
    char *ptr;
    struct sockaddr_in clientAddress = args->clientAddress;
    socklen_t addrLength = sizeof(struct sockaddr);
    for (int i = 0; i < 6; i++) {
        input[i] = strtod(ptrEnd, &ptr);
        if (ptrEnd == ptr) {
            if (sendto(sock, "\nERROR 11: Problem with input. Check amount and type\n",
                       strlen("\nERROR 11: Problem with input. Check amount and type\n"), 0,
                       (const struct sockaddr *) &clientAddress,
                       addrLength) == -1) {
                error("\nERROR 10: Problem with sending\n");
            }
        }
        ptrEnd = ptr;
    }
    double trArea =
            0.5 * fabs((input[2] - input[0]) * (input[5] - input[1]) - (input[4] - input[0]) * (input[3] - input[1]));
    char *trAreaText = (char *) malloc(sizeof(trArea));
    sprintf(trAreaText, "%f\n", trArea);
    if (sendto(sock, trAreaText, strlen(trAreaText), 0,
               (const struct sockaddr *) &clientAddress,
               addrLength) == -1) {
        error("\nERROR 10: Problem with sending\n");
    }
    fprintf(logFile, "\n%s\tMessage: %s sent to client (%s , %d)\n", currentTimestamp(), trAreaText,
            inet_ntoa(clientAddress.sin_addr),
            ntohs(clientAddress.sin_port));
    fflush(logFile);
    count++;
    free(trAreaText);
    pthread_mutex_unlock(&lock);
    pthread_exit(NULL);

}


void serverHandler(int delay) {
    if (pthread_mutex_init(&lock, NULL) != 0) {
        error("\nERROR 54: Mutex init failed\n");
    }
    socklen_t addrLength;
    ssize_t messLength;
    struct sockaddr_in serverAddress;
    int optval = 1;
    pthread_t tid; /* идентификатор потока */
    pthread_attr_t attr; /* атрибуты потока */

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        error("ERROR 73: Socket failed");
    }
    fprintf(logFile, "%s\tSocket created\n", currentTimestamp());
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET; //IP4 internetwork: UDP, TCP, etc.
    serverAddress.sin_port = htons(serverPort); //Устанавливаем порт
    serverAddress.sin_addr.s_addr = inet_addr(serverIP); //00000
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const void *) &optval, sizeof(int));
    if (bind(sock, (struct sockaddr *) &serverAddress,
             sizeof(struct sockaddr)) == -1) {
        error("ERROR 42: Binding failed");
    }
    fprintf(logFile, "%s\tBinding done\n", currentTimestamp());
    addrLength = sizeof(struct sockaddr);
    pthread_attr_init(&attr);

    fprintf(logFile, "%s\tUDP-Server Waiting for client on port %d\n", currentTimestamp(), serverPort);

    fflush(logFile);
    while ("You shall not stop") {
        struct arg_struct args;

        memset(&args, 0, sizeof(struct arg_struct));
        messLength = recvfrom(sock, NULL, 0, MSG_PEEK|MSG_TRUNC,
                              (struct sockaddr *) &args.clientAddress, &addrLength);
        args.data = (char *) malloc(messLength);
        if (args.data == NULL) {
            error("ERROR 52: Malloc failed");
        }
        recvfrom(sock, args.data, messLength, 0,
                 (struct sockaddr *) &args.clientAddress, &addrLength);
        args.data[messLength] = '\0';
        sleep(delay);
        fprintf(logFile, "\n%s\tMessage %s received from(%s , %d)\n", currentTimestamp(), args.data,
                inet_ntoa(args.clientAddress.sin_addr),
                ntohs(args.clientAddress.sin_port));
        pthread_create(&tid, NULL, &triangleSquare, (void *) &args);
        fflush(logFile);
        pthread_join(tid, NULL);
    }
}


int main(int argc, char *argv[]) {
    char *help = "\033[1mNAME\033[0m\n\t14var2Server - UDP server, which calculates area of a triangle from coordinates\n"
                 "\033[1mSYNOPSIS\033[0m\n\t 14var2Server [OPTIONS]\n"
                 "\033[1mDESCRIPTION\033[0m\n"
                 "\t-w=N\n\t\tset delay N for client \n"
                 "\t-d\n\t\tdaemon mode\n"
                 "\t-l=path/to/log\n\t\tPath to log-file\n"
                 "\t-a=IP\n\t\tset server listening IP\n"
                 "\t-p=PORT\n\t\tset server listening PORT\n"
                 "\t-v\n\t\tcheck program version\n"
                 "\t-h\n\t\tprint help\n";
    int rez;
    int delay = 0;
    int daemonFlag = 0;
    char *logPath = "/tmp/lab2.log";
    begin = clock();
    signal(SIGINT, quit);
    signal(SIGTERM, quit);
    signal(SIGQUIT, quit);
    signal(SIGUSR1, quitWithLog);
    if (getenv("L2ADDR") != NULL) {
        serverIP = getenv("L2ADDR");
    }
    if (getenv("L2PORT") != NULL) {
        serverPort = atoi(getenv("L2PORT"));
    }
    if (getenv("L2WAIT") != NULL) {
        delay = atoi(getenv("L2WAIT"));
    }
    while ((rez = getopt(argc, argv, "w:dl:a:p:vh")) != -1) {
        switch (rez) {
            case 'w':
                delay = atoi(optarg);
                break;
            case 'd':
                daemonFlag = 1;
                break;
            case 'l':
                if (strncmp(optarg, "", 1) == 0) {
                    printf("%s", help);
                    return 0;
                }
                logPath = strdup(optarg);
                break;
            case 'a':
                if (strncmp(optarg, "", 1) == 0) {
                    printf("%s", help);
                    return 0;
                }
                serverIP = optarg;
                break;
            case 'p':
                if (strncmp(optarg, "", 1) == 0) {
                    printf("%s", help);
                    return 0;
                }
                serverPort = atoi(optarg);
                break;
            case 'v':
                printf("\033[1m14var2Server version 1.9 \"myBulochka\"\033[0m");
                return 0;
            case 'h': //далее ключи для справки
            case '?':
            default:
                printf("%s", help);
                return 0;
        }
    }

    if (daemonFlag == 1) {
        pid_t pid, sid;
        pid = fork();
        if (pid < 0) {
            error("ERROR 99: Fork failed ");
        }
        if (pid > 0) {
            exit(0);
        }
        umask(0);

        if ((logFile = fopen(logPath, "rb+")) == NULL) {
            error("ERROR 24: Open file");
        }

        sid = setsid();
        if (sid < 0) {
            error("ERROR 98: SID failed ");
        }
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
        serverHandler(delay);
        pthread_mutex_destroy(&lock);

    } else {
        if ((logFile = fopen(logPath, "a+b")) == NULL) {
            error("ERROR 24: Open file");
        }
        serverHandler(delay);
        pthread_mutex_destroy(&lock);
    }
}