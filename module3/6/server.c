#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#define SERVER_PRIORITY 10
#define MAX_CLIENTS 10
#define MAX_TEXT_SIZE 256

struct message {
    long mtype;
    char mtext[MAX_TEXT_SIZE];
    int client_id;
};

struct client {
    int pid;
    int client_id;
    int priority;
};

int msgid;
struct client clients[MAX_CLIENTS];
int num_clients = 0;
int running = 1;

void add_client(int pid, int client_id, int priority) {
    for (int i = 0; i < num_clients; i++) {
        if (clients[i].client_id == client_id) {
            printf("Клиент с ID %d уже зарегистрирован\n", client_id);
            return;
        }
    }
    if (num_clients < MAX_CLIENTS) {
        clients[num_clients].pid = pid;
        clients[num_clients].client_id = client_id;
        clients[num_clients].priority = priority;
        num_clients++;
        printf("Клиент добавлен: PID=%d, ID=%d, Приоритет=%d\n", pid, client_id, priority);
    }
}

void remove_client(int pid) {
    for (int i = 0; i < num_clients; i++) {
        if (clients[i].pid == pid) {
            printf("Клиент удален: PID=%d, ID=%d\n", pid, clients[i].client_id);
            for (int j = i; j < num_clients - 1; j++) {
                clients[j] = clients[j + 1];
            }
            num_clients--;
            break;
        }
    }
}

struct client* find_client_by_id(int client_id) {
    for (int i = 0; i < num_clients; i++) {
        if (clients[i].client_id == client_id) {
            return &clients[i];
        }
    }
    return NULL;
}

void forward_message(struct message msg, int sender_client_id) {
    for (int i = 0; i < num_clients; i++) {
        if (clients[i].client_id != sender_client_id) {
            struct message forward_msg;
            forward_msg.mtype = clients[i].priority;
            snprintf(forward_msg.mtext, MAX_TEXT_SIZE, "От клиента %d: %s", 
                    sender_client_id, msg.mtext);
            forward_msg.client_id = msg.client_id;
            
            if (msgsnd(msgid, &forward_msg, sizeof(forward_msg) - sizeof(long), 0) == -1) {
                perror("msgsnd");
            } else {
                printf("Сообщение переслано клиенту %d (Приоритет %d)\n", 
                       clients[i].client_id, clients[i].priority);
            }
        }
    }
}

void handle_sigint(int sig) {
    running = 0;
}

int main() {
    key_t key;
    struct message msg;
    setbuf(stdout, NULL);
    
    key = ftok("/tmp", 'A');
    if (key == -1) {
        perror("ftok");
        exit(1);
    }
    
    msgid = msgget(key, 0666 | IPC_CREAT);
    if (msgid == -1) {
        perror("msgget");
        exit(1);
    }
    
    struct sigaction sa;
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa, NULL);
    
    printf("Сервер запущен. ID очереди: %d\n", msgid);
    printf("Приоритет сервера: %d\n", SERVER_PRIORITY);
    
    while (running) {
        if (msgrcv(msgid, &msg, sizeof(msg) - sizeof(long), SERVER_PRIORITY, 0) == -1) {
            if (errno == EINTR) {
                continue;
            }
            perror("msgrcv");
            exit(1);
        }
        
        if (strncmp(msg.mtext, "REGISTER:", 9) == 0) {
            int client_id, priority, pid;
            sscanf(msg.mtext, "REGISTER:%d:%d:%d", &client_id, &priority, &pid);
            add_client(pid, client_id, priority);
            continue;
        }
        
        if (strcmp(msg.mtext, "SHUTDOWN") == 0) {
            struct client* client = find_client_by_id(msg.client_id);
            if (client != NULL) {
                printf("Получен запрос на завершение от клиента %d (PID %d)\n", 
                       client->client_id, client->pid);
                remove_client(client->pid);
            }
            continue;
        }
        
        printf("Получено сообщение от клиента %d: %s\n", msg.client_id, msg.mtext);
        forward_message(msg, msg.client_id);
    }
    
    msgctl(msgid, IPC_RMID, NULL);
    printf("Сервер остановлен.\n");
    
    return 0;
}
