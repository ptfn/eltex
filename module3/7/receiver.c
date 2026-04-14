#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#define MAX_MSG_SIZE 256
#define TERMINATE_PRIORITY 10

int main() {
    mqd_t send_desc, recv_desc;
    char send_queue_name[] = "/receive_queue";
    char receive_queue_name[] = "/send_queue";
    char buffer[MAX_MSG_SIZE];
    unsigned priority;
    struct mq_attr attr;
    
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = MAX_MSG_SIZE;
    attr.mq_curmsgs = 0;
    
    recv_desc = mq_open(receive_queue_name, O_RDONLY | O_CREAT, 0644, &attr);
    if (recv_desc == -1) {
        perror("mq_open receive_queue");
        exit(EXIT_FAILURE);
    }
    
    send_desc = mq_open(send_queue_name, O_WRONLY | O_CREAT, 0644, &attr);
    if (send_desc == -1) {
        perror("mq_open send_queue");
        mq_close(recv_desc);
        exit(EXIT_FAILURE);
    }
    
    printf("Получатель запущен. Ожидание сообщений...\n");
    
    while (1) {
        ssize_t bytes_read = mq_receive(recv_desc, buffer, MAX_MSG_SIZE, &priority);
        if (bytes_read == -1) {
            perror("mq_receive");
            continue;
        }
        
        buffer[bytes_read] = '\0';
        
        if (priority == TERMINATE_PRIORITY && strcmp(buffer, "TERMINATE") == 0) {
            printf("Отправитель запросил завершение обмена.\n");
            
            if (mq_send(send_desc, "TERMINATE", 10, TERMINATE_PRIORITY) == -1) {
                perror("mq_send terminate");
            }
            break;
        }
        
        printf("Отправитель: %s\n", buffer);
        
        char response[MAX_MSG_SIZE];
        printf("Получитель: ");
        fgets(response, MAX_MSG_SIZE, stdin);
        response[strcspn(response, "\n")] = '\0';
        
        if (strcmp(response, "exit") == 0) {
            if (mq_send(send_desc, "TERMINATE", 10, TERMINATE_PRIORITY) == -1) {
                perror("mq_send terminate");
            }
            break;
        }
        
        if (mq_send(send_desc, response, strlen(response), 1) == -1) {
            perror("mq_send");
            continue;
        }
    }
    
    mq_close(send_desc);
    mq_close(recv_desc);
    
    mq_unlink(send_queue_name);
    mq_unlink(receive_queue_name);
    
    return 0;
}
