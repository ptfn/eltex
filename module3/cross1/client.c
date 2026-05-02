#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

#define BUFFER_SIZE 4096

int sockfd;
struct sockaddr_in server_addr;
volatile int running = 1;

void signal_handler(int sig) {
    printf("\nReceived signal %d, sending CLOSE to server...\n", sig);
    running = 0;
    
    // Отправляем сообщение о закрытии
    char packet[BUFFER_SIZE];
    memset(packet, 0, BUFFER_SIZE);
    
    struct iphdr *ip = (struct iphdr *)packet;
    struct udphdr *udp = (struct udphdr *)(packet + sizeof(struct iphdr));
    char *payload = packet + sizeof(struct iphdr) + sizeof(struct udphdr);
    
    const char *msg = "CLOSE";
    int msg_len = strlen(msg);
    memcpy(payload, msg, msg_len);
    
    ip->ihl = 5;
    ip->version = 4;
    ip->tos = 0;
    ip->tot_len = htons(sizeof(struct iphdr) + sizeof(struct udphdr) + msg_len);
    ip->id = htons(12345);
    ip->frag_off = 0;
    ip->ttl = 64;
    ip->protocol = IPPROTO_UDP;
    ip->check = 0;
    ip->saddr = inet_addr("127.0.0.1");
    ip->daddr = server_addr.sin_addr.s_addr;
    
    unsigned int sum = 0;
    unsigned short *ptr = (unsigned short *)ip;
    for (int i = 0; i < 10; i++) sum += ntohs(ptr[i]);
    while (sum > 0xFFFF) sum = (sum & 0xFFFF) + (sum >> 16);
    ip->check = htons(~sum);
    
    udp->source = htons(0);
    udp->dest = htons(7777);
    udp->len = htons(sizeof(struct udphdr) + msg_len);
    udp->check = 0;
    
    sendto(sockfd, packet, sizeof(struct iphdr) + sizeof(struct udphdr) + msg_len, 0,
           (struct sockaddr *)&server_addr, sizeof(server_addr));
    
    close(sockfd);
    exit(0);
}

unsigned short checksum(void *data, int len) {
    unsigned short *buf = data;
    unsigned int sum = 0;
    while (len > 1) {
        sum += *buf++;
        len -= 2;
    }
    if (len == 1) sum += *(unsigned char *)buf;
    sum = (sum >> 16) + (sum & 0xFFFF);
    return (unsigned short)(~sum);
}

int main(int argc, char *argv[]) {
    char buffer[BUFFER_SIZE];
    
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server_ip> <server_port>\n", argv[0]);
        exit(1);
    }
    
    const char *server_ip = argv[1];
    int server_port = atoi(argv[2]);
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
    if (sockfd < 0) {
        perror("socket failed (need root)");
        exit(1);
    }
    
    int one = 1;
    if (setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one)) < 0) {
        perror("setsockopt failed");
        exit(1);
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    inet_pton(AF_INET, server_ip, &server_addr.sin_addr);
    
    printf("Raw socket Echo Client started\n");
    printf("Connected to %s:%d\n", server_ip, server_port);
    printf("Type messages to send (Ctrl+C to quit):\n\n");
    
    while (running) {
        printf("> ");
        fflush(stdout);
        
        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
            break;
        }
        
        // Убираем перевод строки
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len-1] == '\n') {
            buffer[len-1] = '\0';
            len--;
        }
        
        if (len == 0) continue;
        
        if (strcmp(buffer, "exit") == 0 || strcmp(buffer, "quit") == 0) {
            printf("Sending CLOSE to server...\n");
            break;
        }
        
        // Создаём сырой пакет
        char packet[BUFFER_SIZE];
        memset(packet, 0, BUFFER_SIZE);
        
        struct iphdr *ip = (struct iphdr *)packet;
        struct udphdr *udp = (struct udphdr *)(packet + sizeof(struct iphdr));
        char *payload = packet + sizeof(struct iphdr) + sizeof(struct udphdr);
        
        memcpy(payload, buffer, len);
        
        ip->ihl = 5;
        ip->version = 4;
        ip->tos = 0;
        ip->tot_len = htons(sizeof(struct iphdr) + sizeof(struct udphdr) + len);
        ip->id = htons(54321);
        ip->frag_off = 0;
        ip->ttl = 64;
        ip->protocol = IPPROTO_UDP;
        ip->saddr = inet_addr("127.0.0.1");
        ip->daddr = server_addr.sin_addr.s_addr;
        
        ip->check = checksum(ip, sizeof(struct iphdr));
        
        udp->source = htons(0);
        udp->dest = htons(server_port);
        udp->len = htons(sizeof(struct udphdr) + len);
        udp->check = 0;
        
        ssize_t sent = sendto(sockfd, packet, sizeof(struct iphdr) + sizeof(struct udphdr) + len, 0,
                             (struct sockaddr *)&server_addr, sizeof(server_addr));
        
        if (sent < 0) {
            perror("sendto failed");
            continue;
        }
        
        // Ждём ответ
        memset(buffer, 0, BUFFER_SIZE);
        struct sockaddr_in from_addr;
        socklen_t from_len = sizeof(from_addr);
        
        ssize_t n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
                             (struct sockaddr *)&from_addr, &from_len);
        
        if (n > 0) {
            struct iphdr *resp_ip = (struct iphdr *)buffer;
            struct udphdr *resp_udp = (struct udphdr *)(buffer + sizeof(struct iphdr));
            char *resp_payload = buffer + sizeof(struct iphdr) + sizeof(struct udphdr);
            
            int payload_len = n - sizeof(struct iphdr) - sizeof(struct udphdr);
            if (payload_len > 0) {
                resp_payload[payload_len] = '\0';
                printf("Server: %s\n", resp_payload);
            }
        }
    }
    
    close(sockfd);
    printf("Client closed.\n");
    return 0;
}