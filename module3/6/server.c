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
#define MAX_TEXT_SIZE 100

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

struct client* find_client(int pid) {
    for (int i = 0; i < num_clients; i++) {
        if (clients[i].pid == pid) {
            return &clients[i];
        }
    }
    return NULL;
}

void forward_message(struct message msg, int sender_pid) {
    for (int i = 0; i < num_clients; i++) {
        if (clients[i].pid != sender_pid) {
            struct message forward_msg;
            forward_msg.mtype = clients[i].priority;
            snprintf(forward_msg.mtext, MAX_TEXT_SIZE, "От клиента %d (PID %d): %s", 
                    msg.client_id, sender_pid, msg.mtext);
            forward_msg.client_id = msg.client_id;
            
            if (msgsnd(msgid, &forward_msg, sizeof(forward_msg.mtext), 0) == -1) {
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
    int client_pid;
    
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
    
    signal(SIGINT, handle_sigint);
    
    printf("Сервер запущен. ID очереди: %d\n", msgid);
    printf("Приоритет сервера: %d\n", SERVER_PRIORITY);
    
    while (running) {
        if (msgrcv(msgid, &msg, MAX_TEXT_SIZE, 0, 0) == -1) {
            if (errno == EINTR) {
                continue;
            }
            perror("msgrcv");
            exit(1);
        }
        
        client_pid = msg.mtype;
        
        if (strncmp(msg.mtext, "REGISTER:", 9) == 0) {
            int client_id, priority;
            sscanf(msg.mtext, "REGISTER:%d:%d", &client_id, &priority);
            add_client(client_pid, client_id, priority);
            continue;
        }
        
        if (strcmp(msg.mtext, "SHUTDOWN") == 0) {
            struct client* client = find_client(client_pid);
            if (client != NULL) {
                printf("Получен запрос на завершение от клиента %d (PID %d)\n", 
                       client->client_id, client_pid);
                remove_client(client_pid);
            }
            continue;
        }
        
        printf("Получено сообщение от PID %d: %s\n", client_pid, msg.mtext);
        forward_message(msg, client_pid);
    }
    
    msgctl(msgid, IPC_RMID, NULL);
    printf("Сервер остановлен.\n");
    
    return 0;
}
