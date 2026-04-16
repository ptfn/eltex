#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int sockfd;
    struct sockaddr_in server_addr, client1_addr, client2_addr;
    socklen_t addr_len = sizeof(struct sockaddr_in);
    char buffer[BUFFER_SIZE];
    int has_client1 = 0, has_client2 = 0;
    
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client1_addr, 0, sizeof(client1_addr));
    memset(&client2_addr, 0, sizeof(client2_addr));
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    
    printf("Server listening on port %d...\n", PORT);
    
    while (1) {
        struct sockaddr_in sender_addr;
        socklen_t sender_len = sizeof(sender_addr);
        memset(buffer, 0, BUFFER_SIZE);
        
        int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
                         (struct sockaddr *)&sender_addr, &sender_len);
        if (n < 0) {
            perror("recvfrom error");
            continue;
        }
        
        buffer[n] = '\0';
        
        if (!has_client1) {
            client1_addr = sender_addr;
            has_client1 = 1;
            printf("Client 1 connected: %s:%d\n",
                   inet_ntoa(client1_addr.sin_addr), ntohs(client1_addr.sin_port));
            char *welcome = "You are client 1. Waiting for client 2...\n";
            sendto(sockfd, welcome, strlen(welcome), 0,
                   (struct sockaddr *)&client1_addr, sizeof(client1_addr));
        } else if (!has_client2) {
            if (sender_addr.sin_addr.s_addr == client1_addr.sin_addr.s_addr &&
                sender_addr.sin_port == client1_addr.sin_port) {
                char *msg = "Waiting for client 2 to join...\n";
                sendto(sockfd, msg, strlen(msg), 0,
                       (struct sockaddr *)&client1_addr, sizeof(client1_addr));
                continue;
            }
            client2_addr = sender_addr;
            has_client2 = 1;
            printf("Client 2 connected: %s:%d\n",
                   inet_ntoa(client2_addr.sin_addr), ntohs(client2_addr.sin_port));
            char *msg1 = "Client 2 has joined. You can start chatting!\n";
            char *msg2 = "You are client 2. You can start chatting!\n";
            sendto(sockfd, msg1, strlen(msg1), 0,
                   (struct sockaddr *)&client1_addr, sizeof(client1_addr));
            sendto(sockfd, msg2, strlen(msg2), 0,
                   (struct sockaddr *)&client2_addr, sizeof(client2_addr));
        } else {
            struct sockaddr_in *dest_addr;
            if (sender_addr.sin_addr.s_addr == client1_addr.sin_addr.s_addr &&
                sender_addr.sin_port == client1_addr.sin_port) {
                dest_addr = &client2_addr;
                printf("Message from client 1 to client 2: %s", buffer);
            } else if (sender_addr.sin_addr.s_addr == client2_addr.sin_addr.s_addr &&
                       sender_addr.sin_port == client2_addr.sin_port) {
                dest_addr = &client1_addr;
                printf("Message from client 2 to client 1: %s", buffer);
            } else {
                printf("Unknown client %s:%d sent: %s\n",
                       inet_ntoa(sender_addr.sin_addr), ntohs(sender_addr.sin_port), buffer);
                continue;
            }
            
            sendto(sockfd, buffer, n, 0, (struct sockaddr *)dest_addr, sizeof(*dest_addr));
        }
    }
    
    close(sockfd);
    return 0;
}
