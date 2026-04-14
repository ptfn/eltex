#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <errno.h>
#include <errno.h>
#include <signal.h>

#define SERVER_PRIORITY 10
#define MAX_TEXT_SIZE 100

struct message {
    long mtype;
    char mtext[MAX_TEXT_SIZE];
    int client_id;
};

int msgid;
int client_id;
int client_priority;
int running = 1;

void send_to_server(const char* text) {
    struct message msg;
    msg.mtype = getpid();
    strncpy(msg.mtext, text, MAX_TEXT_SIZE);
    msg.client_id = client_id;
    
    if (msgsnd(msgid, &msg, sizeof(msg.mtext), 0) == -1) {
        perror("msgsnd");
    }
}

void receive_messages() {
    struct message msg;
    
    if (msgrcv(msgid, &msg, MAX_TEXT_SIZE, client_priority, 0) == -1) {
        if (errno == EINTR) {
            return;
        }
        perror("msgrcv");
        exit(1);
    }
    
    printf("Получено сообщение: %s\n", msg.mtext);
}

void handle_sigint(int sig) {
    running = 0;
}

int main(int argc, char* argv[]) {
    key_t key;
    char input[MAX_TEXT_SIZE];
    
    if (argc != 3) {
        printf("Использование: %s <client_id> <client_priority>\n", argv[0]);
        exit(1);
    }
    
    client_id = atoi(argv[1]);
    client_priority = atoi(argv[2]);
    
    printf("Клиент запущен. ID клиента: %d, Приоритет: %d\n", client_id, client_priority);
    
    key = ftok("/tmp", 'A');
    if (key == -1) {
        perror("ftok");
        exit(1);
    }
    
    msgid = msgget(key, 0666);
    if (msgid == -1) {
        perror("msgget");
        exit(1);
    }
    
    char reg_msg[MAX_TEXT_SIZE];
    snprintf(reg_msg, MAX_TEXT_SIZE, "REGISTER:%d:%d", client_id, client_priority);
    send_to_server(reg_msg);
    
    signal(SIGINT, handle_sigint);
    
    printf("Клиент зарегистрирован. Введите сообщения для отправки (или 'SHUTDOWN' для выхода):\n");
    
    while (running) {
        printf("> ");
        fflush(stdout);
        
        if (fgets(input, MAX_TEXT_SIZE, stdin) == NULL) {
            break;
        }
        
        input[strcspn(input, "\n")] = '\0';
        
        if (strcmp(input, "SHUTDOWN") == 0) {
            send_to_server("SHUTDOWN");
            break;
        }
        
        send_to_server(input);
        receive_messages();
    }
    
    printf("Клиент остановлен.\n");
    
    return 0;
}
