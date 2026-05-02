#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <time.h>

#define PORT 8080
#define BUFFER_SIZE 2048

#define IP_HDR_LEN 20
#define UDP_HDR_LEN 8

void print_hex_dump(FILE *fp, const char *data, int len) {
    for (int i = 0; i < len; i++) {
        fprintf(fp, "%02x ", (unsigned char)data[i]);
        if ((i + 1) % 16 == 0) fprintf(fp, "\n");
        else if ((i + 1) % 8 == 0) fprintf(fp, " ");
    }
    if (len % 16 != 0) fprintf(fp, "\n");
}

void print_ascii_dump(FILE *fp, const char *data, int len) {
    for (int i = 0; i < len; i++) {
        if (data[i] >= 32 && data[i] <= 126) {
            fprintf(fp, "%c", data[i]);
        } else {
            fprintf(fp, ".");
        }
    }
    fprintf(fp, "\n");
}

int main() {
    int sockfd;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in sender_addr;
    socklen_t addr_len = sizeof(sender_addr);
    
    FILE *dump_file = fopen("network_dump.bin", "wb");
    FILE *log_file = fopen("sniffer_log.txt", "w");
    if (!dump_file || !log_file) {
        perror("Failed to open output files");
        return 1;
    }
    
    sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
    if (sockfd < 0) {
        perror("socket creation failed (need root or CAP_NET_RAW)");
        fprintf(log_file, "Error: Need root privileges or CAP_NET_RAW\n");
        fclose(dump_file);
        fclose(log_file);
        return 1;
    }
    
    int one = 1;
    if (setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one)) < 0) {
        perror("setsockopt IP_HDRINCL failed");
        close(sockfd);
        fclose(dump_file);
        fclose(log_file);
        return 1;
    }
    
    printf("Sniffer started. Listening for UDP packets on port %d...\n", PORT);
    printf("Press Ctrl+C to stop.\n");
    fprintf(log_file, "Sniffer log started at port %d\n", PORT);
    fprintf(log_file, "----------------------------------------\n");
    
    int packet_count = 0;
    
    while (1) {
        ssize_t n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
                             (struct sockaddr *)&sender_addr, &addr_len);
        
        if (n < 0) {
            perror("recvfrom error");
            continue;
        }
        
        if (n < IP_HDR_LEN + UDP_HDR_LEN) {
            continue;
        }
        
        struct iphdr *ip_hdr = (struct iphdr *)buffer;
        
        if (ip_hdr->protocol != IPPROTO_UDP) {
            continue;
        }
        
        struct udphdr *udp_hdr = (struct udphdr *)(buffer + IP_HDR_LEN);
        
        uint16_t src_port = ntohs(udp_hdr->source);
        uint16_t dst_port = ntohs(udp_hdr->dest);
        
        if (dst_port == PORT) {
            packet_count++;
            
            time_t now = time(NULL);
            char time_str[64];
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&now));
            
            char src_ip[INET_ADDRSTRLEN];
            char dst_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &ip_hdr->saddr, src_ip, INET_ADDRSTRLEN);
            inet_ntop(AF_INET, &ip_hdr->daddr, dst_ip, INET_ADDRSTRLEN);
            
            printf("\n=== Packet #%d ===\n", packet_count);
            printf("Time: %s\n", time_str);
            printf("Source: %s:%d\n", src_ip, src_port);
            printf("Dest: %s:%d\n", dst_ip, dst_port);
            printf("Total size: %zd bytes\n", n);
            
            fprintf(log_file, "\n=== Packet #%d ===\n", packet_count);
            fprintf(log_file, "Time: %s\n", time_str);
            fprintf(log_file, "Source: %s:%d\n", src_ip, src_port);
            fprintf(log_file, "Dest: %s:%d\n", dst_ip, dst_port);
            fprintf(log_file, "Total size: %zd bytes\n", n);
            
            fwrite(buffer, 1, n, dump_file);
            fflush(dump_file);
            
            printf("\n--- Raw hex dump ---\n");
            print_hex_dump(stdout, buffer, n);
            
            printf("\n--- Hex dump (log) ---\n");
            print_hex_dump(log_file, buffer, n);
            
            int ip_header_len = ip_hdr->ihl * 4;
            int udp_len = ntohs(udp_hdr->len);
            int payload_len = udp_len - UDP_HDR_LEN;
            
            if (payload_len > 0) {
                char *payload = (char *)(buffer + IP_HDR_LEN + UDP_HDR_LEN);
                
                printf("\n--- UDP Payload (decoded message) ---\n");
                printf("%.*s\n", payload_len, payload);
                
                fprintf(log_file, "\n--- UDP Payload (decoded message) ---\n");
                fprintf(log_file, "%.*s\n", payload_len, payload);
                
                printf("\n--- ASCII dump of payload ---\n");
                print_ascii_dump(stdout, payload, payload_len);
                
                fprintf(log_file, "\n--- ASCII dump of payload ---\n");
                print_ascii_dump(log_file, payload, payload_len);
            }
            
            printf("\n--- IP Header ---\n");
            printf("Version: %d\n", ip_hdr->version);
            printf("Header Length: %d bytes\n", ip_header_len);
            printf("Total Length: %d bytes\n", ntohs(ip_hdr->tot_len));
            printf("Protocol: %d (UDP)\n", ip_hdr->protocol);
            
            fprintf(log_file, "\n--- IP Header ---\n");
            fprintf(log_file, "Version: %d\n", ip_hdr->version);
            fprintf(log_file, "Header Length: %d bytes\n", ip_header_len);
            fprintf(log_file, "Total Length: %d bytes\n", ntohs(ip_hdr->tot_len));
            fprintf(log_file, "Protocol: %d (UDP)\n", ip_hdr->protocol);
            
            printf("\n--- UDP Header ---\n");
            printf("Source Port: %d\n", src_port);
            printf("Dest Port: %d\n", dst_port);
            printf("UDP Length: %d bytes\n", udp_len);
            
            fprintf(log_file, "\n--- UDP Header ---\n");
            fprintf(log_file, "Source Port: %d\n", src_port);
            fprintf(log_file, "Dest Port: %d\n", dst_port);
            fprintf(log_file, "UDP Length: %d bytes\n", udp_len);
            
            fprintf(log_file, "\n----------------------------------------\n");
            fflush(log_file);
        }
    }
    
    close(sockfd);
    fclose(dump_file);
    fclose(log_file);
    
    return 0;
}