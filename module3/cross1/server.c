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
#define PORT 7777

typedef struct {
    char ip[16];
    int port;
    int counter;
} client_t;

client_t clients[64];
int client_count = 0;

void handle_signal(int sig) {
    printf("\nServer received signal %d, shutting down...\n", sig);
    exit(0);
}

int find_client(const char *ip, int port) {
    for (int i = 0; i < client_count; i++) {
        if (strcmp(clients[i].ip, ip) == 0 && clients[i].port == port)
            return i;
    }
    return -1;
}

int add_client(const char *ip, int port) {
    if (client_count >= 64) return -1;
    strcpy(clients[client_count].ip, ip);
    clients[client_count].port = port;
    clients[client_count].counter = 0;
    return client_count++;
}

void remove_client(const char *ip, int port) {
    int idx = find_client(ip, port);
    if (idx >= 0) {
        for (int i = idx; i < client_count - 1; i++)
            clients[i] = clients[i + 1];
        client_count--;
    }
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

void build_ip_header(struct iphdr *ip, int payload_len, const char *src_ip, const char *dst_ip) {
    ip->ihl = 5;
    ip->version = 4;
    ip->tos = 0;
    ip->tot_len = htons(sizeof(struct iphdr) + sizeof(struct udphdr) + payload_len);
    ip->id = htons(54321);
    ip->frag_off = 0;
    ip->ttl = 64;
    ip->protocol = IPPROTO_UDP;
    ip->check = 0;
    inet_pton(AF_INET, src_ip, &ip->saddr);
    inet_pton(AF_INET, dst_ip, &ip->daddr);
    ip->check = checksum(ip, sizeof(struct iphdr));
}

void build_udp_header(struct udphdr *udp, int src_port, int dst_port, int payload_len) {
    udp->source = htons(src_port);
    udp->dest = htons(dst_port);
    udp->len = htons(sizeof(struct udphdr) + payload_len);
    udp->check = 0;
}

int main() {
    int sockfd;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    
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
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        exit(1);
    }
    
    printf("Raw socket Echo Server started on port %d\n", PORT);
    printf("Waiting for messages...\n");
    
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        ssize_t n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
                             (struct sockaddr *)&client_addr, &addr_len);
        if (n <= 0) continue;
        
        if (n < sizeof(struct iphdr) + sizeof(struct udphdr)) continue;
        
        struct iphdr *ip = (struct iphdr *)buffer;
        struct udphdr *udp = (struct udphdr *)(buffer + sizeof(struct iphdr));
        
        if (ip->protocol != IPPROTO_UDP) continue;
        
        char src_ip[INET_ADDRSTRLEN], dst_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &ip->saddr, src_ip, INET_ADDRSTRLEN);
        inet_ntop(AF_INET, &ip->daddr, dst_ip, INET_ADDRSTRLEN);
        
        int src_port = ntohs(udp->source);
        int dst_port = ntohs(udp->dest);
        
        int payload_len = ntohs(udp->len) - sizeof(struct udphdr);
        char *payload = (char *)(buffer + sizeof(struct iphdr) + sizeof(struct udphdr));
        
        if (payload_len <= 0 || payload_len >= BUFFER_SIZE - 100) continue;
        
        payload[payload_len] = '\0';
        
        // Убираем перевод строки в конце
        if (payload_len > 0 && (payload[payload_len-1] == '\n' || payload[payload_len-1] == '\r'))
            payload[--payload_len] = '\0';
        
        printf("Received from %s:%d -> %s\n", src_ip, src_port, payload);
        
        // Проверяем, это сообщение о закрытии
        if (strcmp(payload, "CLOSE") == 0) {
            remove_client(src_ip, src_port);
            printf("Client %s:%d closed connection, counters reset\n", src_ip, src_port);
            continue;
        }
        
        // Ищем или добавляем клиента
        int idx = find_client(src_ip, src_port);
        if (idx < 0) {
            idx = add_client(src_ip, src_port);
            printf("New client registered: %s:%d (counter reset)\n", src_ip, src_port);
        }
        
        clients[idx].counter++;
        int counter = clients[idx].counter;
        
        // Формируем ответ
        char response[BUFFER_SIZE];
        snprintf(response, sizeof(response), "%s %d", payload, counter);
        int resp_len = strlen(response);
        
        // Создаём ответный пакет
        char packet[BUFFER_SIZE];
        memset(packet, 0, BUFFER_SIZE);
        
        struct iphdr *resp_ip = (struct iphdr *)packet;
        struct udphdr *resp_udp = (struct udphdr *)(packet + sizeof(struct iphdr));
        char *resp_payload = packet + sizeof(struct iphdr) + sizeof(struct udphdr);
        
        memcpy(resp_payload, response, resp_len);
        
        build_ip_header(resp_ip, sizeof(struct udphdr) + resp_len, dst_ip, src_ip);
        build_udp_header(resp_udp, PORT, src_port, resp_len);
        
        struct sockaddr_in dest;
        memset(&dest, 0, sizeof(dest));
        dest.sin_family = AF_INET;
        dest.sin_port = htons(src_port);
        inet_pton(AF_INET, src_ip, &dest.sin_addr);
        
        ssize_t sent = sendto(sockfd, packet, sizeof(struct iphdr) + sizeof(struct udphdr) + resp_len, 0,
                              (struct sockaddr *)&dest, sizeof(dest));
        
        if (sent > 0) {
            printf("Sent to %s:%d -> %s\n", src_ip, src_port, response);
        }
    }
    
    close(sockfd);
    return 0;
}