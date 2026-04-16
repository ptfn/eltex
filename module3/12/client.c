#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define BUFFER_SIZE 1024

int sockfd;
struct sockaddr_in server_addr;
int running = 1;

void *receive_thread(void *arg) {
    char buffer[BUFFER_SIZE];
    while (running) {
        memset(buffer, 0, BUFFER_SIZE);
        int n = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
        if (n <= 0) {
            if (running) {
                printf("Disconnected from server.\n");
                running = 0;
            }
            break;
        }
        buffer[n] = '\0';
        printf("%s", buffer);
        fflush(stdout);
    }
    return NULL;
}

void *send_thread(void *arg) {
    char buffer[BUFFER_SIZE];
    while (running) {
        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
            break;
        }
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len-1] == '\n') {
            buffer[len-1] = '\0';
        }
        if (strcmp(buffer, "exit") == 0) {
            running = 0;
            break;
        }
        if (send(sockfd, buffer, strlen(buffer), 0) < 0) {
            perror("send failed");
            break;
        }
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server_ip> <server_port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    const char *server_ip = argv[1];
    int server_port = atoi(argv[2]);
    
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("invalid address");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    
    printf("Connected to server %s:%d. Type 'exit' to quit.\n", server_ip, server_port);
    
    pthread_t recv_tid, send_tid;
    
    if (pthread_create(&recv_tid, NULL, receive_thread, NULL) != 0) {
        perror("pthread_create for receive failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    
    if (pthread_create(&send_tid, NULL, send_thread, NULL) != 0) {
        perror("pthread_create for send failed");
        running = 0;
        pthread_join(recv_tid, NULL);
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    
    pthread_join(recv_tid, NULL);
    pthread_join(send_tid, NULL);
    
    close(sockfd);
    printf("Client terminated.\n");
    return 0;
}
