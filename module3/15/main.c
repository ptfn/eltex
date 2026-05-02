#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/select.h>

#define MAX_CLIENTS 64
#define BUFFER_SIZE 1024
#define READLINE_BUF 4096

typedef struct {
    int fd;
    char readbuf[READLINE_BUF];
    int readpos;
    int readlen;
} client_t;

int nclients = 0;
client_t clients[MAX_CLIENTS];

int calculate(char op, int a, int b, int *result)
{
    switch (op)
    {
    case '+': *result = a + b; break;
    case '-': *result = a - b; break;
    case '*': *result = a * b; break;
    case '/':
        if (b == 0) return 0;
        *result = a / b;
        break;
    default: return 0;
    }
    return 1;
}

int send_response(int sockfd, const char *msg)
{
    return write(sockfd, msg, strlen(msg));
}

int readline(int sockfd, char *buf, int maxlen, char *readbuf, int *readpos, int *readlen)
{
    int n = 0;
    while (n < maxlen - 1)
    {
        if (*readpos >= *readlen)
        {
            *readlen = read(sockfd, readbuf, READLINE_BUF);
            *readpos = 0;
            if (*readlen <= 0)
                return -1;
        }
        buf[n++] = readbuf[(*readpos)++];
        if (buf[n-1] == '\n')
            break;
    }
    buf[n] = '\0';
    return n;
}

int readbytes(int sockfd, char *buf, int len, char *readbuf, int *readpos, int *readlen)
{
    int total = 0;
    while (total < len)
    {
        if (*readpos >= *readlen)
        {
            *readlen = read(sockfd, readbuf, READLINE_BUF);
            *readpos = 0;
            if (*readlen <= 0)
                return -1;
        }
        int avail = *readlen - *readpos;
        int tocopy = (len - total < avail) ? len - total : avail;
        memcpy(buf + total, readbuf + *readpos, tocopy);
        *readpos += tocopy;
        total += tocopy;
    }
    return total;
}

void handle_client(client_t *client, fd_set *master_fds)
{
    char buf[BUFFER_SIZE];
    char cmd[32], arg1[256], arg2[256];
    char filename[256];
    
    int n = readline(client->fd, buf, sizeof(buf), client->readbuf, &client->readpos, &client->readlen);
    if (n <= 0)
    {
        printf("Client disconnected\n");
        close(client->fd);
        client->fd = -1;
        nclients--;
        printf("Users online: %d\n", nclients);
        return;
    }
    
    buf[strcspn(buf, "\r\n")] = '\0';
    
    if (strncmp(buf, "QUIT", 4) == 0)
    {
        printf("Client sent QUIT\n");
        close(client->fd);
        client->fd = -1;
        nclients--;
        printf("Users online: %d\n", nclients);
        return;
    }
    else if (strncmp(buf, "CALC", 4) == 0)
    {
        char op;
        int a, b, result;
        if (sscanf(buf, "%s %c %d %d", cmd, &op, &a, &b) != 4)
        {
            send_response(client->fd, "ERROR Invalid CALC format\n");
            return;
        }
        if (!calculate(op, a, b, &result))
        {
            send_response(client->fd, "ERROR Invalid operation or division by zero\n");
            return;
        }
        snprintf(buf, sizeof(buf), "%d\n", result);
        send_response(client->fd, buf);
    }
    else if (strncmp(buf, "PUT", 3) == 0)
    {
        if (sscanf(buf, "%s %s", cmd, filename) != 2)
        {
            send_response(client->fd, "ERROR PUT requires filename\n");
            return;
        }
        
        n = readline(client->fd, buf, sizeof(buf), client->readbuf, &client->readpos, &client->readlen);
        if (n <= 0)
        {
            close(client->fd);
            client->fd = -1;
            nclients--;
            return;
        }
        buf[strcspn(buf, "\r\n")] = '\0';
        long filesize = atol(buf);
        if (filesize <= 0)
        {
            send_response(client->fd, "ERROR Invalid file size\n");
            return;
        }
        
        char localname[300];
        snprintf(localname, sizeof(localname), "uploaded_%s", filename);
        
        int fd = open(localname, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0)
        {
            send_response(client->fd, "ERROR Cannot create file\n");
            return;
        }
        
        char filebuf[4096];
        long remaining = filesize;
        while (remaining > 0)
        {
            int toread = (remaining < sizeof(filebuf)) ? remaining : sizeof(filebuf);
            int rc = readbytes(client->fd, filebuf, toread, client->readbuf, &client->readpos, &client->readlen);
            if (rc <= 0)
            {
                close(fd);
                send_response(client->fd, "ERROR Receiving file data\n");
                goto cleanup_put;
            }
            if (write(fd, filebuf, rc) != rc)
            {
                close(fd);
                send_response(client->fd, "ERROR Writing to file\n");
                goto cleanup_put;
            }
            remaining -= rc;
        }
        close(fd);
        send_response(client->fd, "OK\n");
    cleanup_put:
        ;
    }
    else if (strncmp(buf, "GET", 3) == 0)
    {
        if (sscanf(buf, "%s %s", cmd, filename) != 2)
        {
            send_response(client->fd, "ERROR GET requires filename\n");
            return;
        }
        
        int fd = open(filename, O_RDONLY);
        if (fd < 0)
        {
            send_response(client->fd, "ERROR File not found\n");
            return;
        }
        
        struct stat st;
        fstat(fd, &st);
        long filesize = st.st_size;
        
        snprintf(buf, sizeof(buf), "%ld\n", filesize);
        send_response(client->fd, buf);
        
        char filebuf[4096];
        ssize_t nread;
        while ((nread = read(fd, filebuf, sizeof(filebuf))) > 0)
        {
            if (write(client->fd, filebuf, nread) != nread)
            {
                close(fd);
                goto cleanup_get;
            }
        }
        close(fd);
    cleanup_get:
        ;
    }
    else
    {
        send_response(client->fd, "ERROR Unknown command\n");
    }
}

int main(int argc, char *argv[])
{
    int sockfd, portno;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen;
    
    printf("TCP SERVER DEMO (with select)\n");
    
    if (argc < 2)
    {
        fprintf(stderr, "ERROR, no port provided\n");
        exit(1);
    }
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        perror("ERROR opening socket");
    
    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    bzero((char *)&serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        perror("ERROR on binding");
    
    listen(sockfd, 5);
    
    fd_set master_fds, read_fds;
    FD_ZERO(&master_fds);
    FD_SET(sockfd, &master_fds);
    
    for (int i = 0; i < MAX_CLIENTS; i++)
        clients[i].fd = -1;
    
    int maxfd = sockfd;
    
    while (1)
    {
        read_fds = master_fds;
        struct timeval tv = { .tv_sec = 1, .tv_usec = 0 };
        int activity = select(maxfd + 1, &read_fds, NULL, NULL, &tv);
        
        if (activity < 0)
        {
            perror("select error");
            continue;
        }
        
        if (FD_ISSET(sockfd, &read_fds))
        {
            clilen = sizeof(cli_addr);
            int newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
            if (newsockfd < 0)
            {
                perror("ERROR on accept");
                continue;
            }
            
            if (nclients >= MAX_CLIENTS)
            {
                printf("Max clients reached, rejecting connection\n");
                close(newsockfd);
            }
            else
            {
                for (int i = 0; i < MAX_CLIENTS; i++)
                {
                    if (clients[i].fd == -1)
                    {
                        clients[i].fd = newsockfd;
                        clients[i].readpos = 0;
                        clients[i].readlen = 0;
                        FD_SET(newsockfd, &master_fds);
                        if (newsockfd > maxfd)
                            maxfd = newsockfd;
                        nclients++;
                        printf("New client connected. Users online: %d\n", nclients);
                        char *welcome = "Server ready. Commands: CALC <op> <a> <b>, PUT <file>, GET <file>, QUIT\n";
                        write(newsockfd, welcome, strlen(welcome));
                        break;
                    }
                }
            }
        }
        
        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            if (clients[i].fd != -1 && FD_ISSET(clients[i].fd, &read_fds))
            {
                int old_fd = clients[i].fd;
                handle_client(&clients[i], &master_fds);
                if (clients[i].fd == -1)
                {
                    FD_CLR(old_fd, &master_fds);
                    int max_check = sockfd;
                    for (int j = 0; j < MAX_CLIENTS; j++) {
                        if (clients[j].fd > max_check)
                            max_check = clients[j].fd;
                    }
                    maxfd = max_check;
                }
            }
        }
    }
    
    close(sockfd);
    return 0;
}