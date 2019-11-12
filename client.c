//
// Client part Created by COLT-ADM on 20.10.2019.
//
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>


int sock, count;
char *serverIP = "127.0.0.1";
int serverPort = 8088;
FILE *logFile;

char *readln(FILE *stream) { //Чтение из stdin'a
    static char *str = NULL;
    static size_t i = 0;
    int ch = fgetc(stream);

    if ((ch == '\n') || (ch == EOF)) {
        str = malloc(i + 1);
        str[i] = 0;
    } else {
        i++;
        readln(stream);
        str[--i] = ch;
    }
    return str;
}

void error(char *err) { // Вывод ошибок в терминал
    printf("%s\n", err);
    fflush(stdin);
    perror(err);
    exit(1);
}

int main(int argc, char *argv[]) {
    char *help = "\033[1mNAME\033[0m\n\t14var2Client - Client which is connected to UDP Server (14var2Server) to send him coordinates and receive area of a triangle\n"
                 "\033[1mSYNOPSIS\033[0m\n\t 14var2Client [OPTIONS]\n"
                 "\033[1mDESCRIPTION\033[0m\n"
                 "\t-a=IP\n\t\tset server listening IP\n"
                 "\t-p=PORT\n\t\tset server listening PORT\n"
                 "\t-v\n\t\tcheck program version\n"
                 "\t-h\n\t\tprint help\n";
    int rez;
    char *message = NULL;
    socklen_t addrLength;
    ssize_t messLength;
    struct sockaddr_in serverAddress;
    if (getenv("L2ADDR") != NULL) {
        serverIP = getenv("L2ADDR");
    }
    if (getenv("L2PORT") != NULL) {
        serverPort = atoi(getenv("L2PORT"));
    }

    while ((rez = getopt(argc, argv, "m:a:p:vh")) != -1) { // Обработка ключей терминала
        switch (rez) {
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
            case 'm':
                if (strncmp(optarg, "", 1) == 0) {
                    printf("%s", help);
                    return 0;
                }
                message = optarg;
                break;
            case 'v':
                printf("14var2Client version 0.9 \"myHlebushek\"");
                return 0;
            case 'h': //далее ключи для справки
            case '?':
            default:
                printf("%s", help);
                return 0;
        }
    }

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) { //Создание сокета
        error("ERROR 73: Socket failed");
    }
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET; //IP4 internetwork: UDP, TCP, etc.
    serverAddress.sin_port = htons(serverPort); //Устанавливаем порт сервера
    serverAddress.sin_addr.s_addr = inet_addr(serverIP); // IP адрес сервера
    addrLength = sizeof(struct sockaddr);

    while ("You shall not stop") { // Бесконечный цикл для работы с сервером

        char *answer = NULL;
        if (message == NULL) { // Если не получили сообщение из командной строки, то считать из stdin
            printf("\nWaiting for your coordinates:\nIf you want to exit write \"exit\"\n");
            message = readln(stdin);
            if (strncmp(message, "exit", 4) == 0) {
                printf("Closing connection...\nDone\n");
                close(sock);
                return 0;
            }
        }
        sendto(sock, message, strlen(message), // Посылаем сообщение серверу
               0, (const struct sockaddr *) &serverAddress,
               addrLength);
        messLength = recvfrom(sock, NULL, 0, MSG_PEEK|MSG_TRUNC, // Получаем длину ответа
                              (struct sockaddr *) &serverAddress, &addrLength);
        answer = (char *) malloc(messLength); // Выделяем память
        if (answer == NULL) {
            error("ERROR 52: Malloc failed");
        }
        recvfrom(sock, answer, messLength, 0, // Получаем ответ на наш вопрос от сервера
                 (struct sockaddr *) &serverAddress, &addrLength);
        answer[messLength] = '\0';
        printf("Server answer: %s \n", answer); // Вывод ответа
        message = NULL;
        free(answer);
    }
    close(sock);
    return 0;
}
