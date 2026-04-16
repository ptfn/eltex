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

void dostuff(int);
void error(const char *msg);
void printusers();

int nclients = 0;

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

void printusers()
{
    if (nclients)
        printf("%d user on-line\n", nclients);
    else
        printf("No User on line\n");
}

int readline(int sock, char *buf, int maxlen)
{
    int n = 0;
    char c;
    while (n < maxlen - 1)
    {
        int rc = read(sock, &c, 1);
        if (rc <= 0)
            return -1;
        buf[n++] = c;
        if (c == '\n')
            break;
    }
    buf[n] = '\0';
    return n;
}

int readbytes(int sock, char *buf, int len)
{
    int total = 0;
    while (total < len)
    {
        int n = read(sock, buf + total, len - total);
        if (n <= 0)
            return -1;
        total += n;
    }
    return total;
}

int calculate(char op, int a, int b, int *result)
{
    switch (op)
    {
    case '+':
        *result = a + b;
        break;
    case '-':
        *result = a - b;
        break;
    case '*':
        *result = a * b;
        break;
    case '/':
        if (b == 0)
            return 0;
        *result = a / b;
        break;
    default:
        return 0;
    }
    return 1;
}

void dostuff(int sock)
{
    char buf[1024];
    char cmd[32], arg1[256], arg2[256];
    int n;

    write(sock, "Server ready. Commands: CALC <op> <a> <b>, PUT <file>, GET <file>, QUIT\n", 72);

    while (1)
    {
        n = readline(sock, buf, sizeof(buf));
        if (n <= 0)
            break;
        buf[strcspn(buf, "\r\n")] = '\0';

        if (strncmp(buf, "QUIT", 4) == 0)
        {
            printf("Client requested QUIT\n");
            break;
        }
        else if (strncmp(buf, "CALC", 4) == 0)
        {
            char op;
            int a, b, result;
            if (sscanf(buf, "%s %c %d %d", cmd, &op, &a, &b) != 4)
            {
                write(sock, "ERROR Invalid CALC format\n", 26);
                continue;
            }
            if (!calculate(op, a, b, &result))
            {
                write(sock, "ERROR Invalid operation or division by zero\n", 44);
                continue;
            }
            snprintf(buf, sizeof(buf), "%d\n", result);
            write(sock, buf, strlen(buf));
        }
        else if (strncmp(buf, "PUT", 3) == 0)
        {
            char filename[256];
            if (sscanf(buf, "%s %s", cmd, filename) != 2)
            {
                write(sock, "ERROR PUT requires filename\n", 29);
                continue;
            }

            n = readline(sock, buf, sizeof(buf));
            if (n <= 0)
                break;
            buf[strcspn(buf, "\r\n")] = '\0';
            long filesize = atol(buf);
            if (filesize <= 0)
            {
                write(sock, "ERROR Invalid file size\n", 24);
                continue;
            }

            char localname[300];
            snprintf(localname, sizeof(localname), "uploaded_%s", filename);

            int fd = open(localname, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0)
            {
                write(sock, "ERROR Cannot create file\n", 25);
                continue;
            }

            char filebuf[4096];
            long remaining = filesize;
            while (remaining > 0)
            {
                int toread = (remaining < sizeof(filebuf)) ? remaining : sizeof(filebuf);
                int rc = readbytes(sock, filebuf, toread);
                if (rc <= 0)
                {
                    close(fd);
                    write(sock, "ERROR Receiving file data\n", 26);
                    goto cleanup_put;
                }
                if (write(fd, filebuf, rc) != rc)
                {
                    close(fd);
                    write(sock, "ERROR Writing to file\n", 22);
                    goto cleanup_put;
                }
                remaining -= rc;
            }
            close(fd);
            write(sock, "OK\n", 3);
        cleanup_put:
            continue;
        }
        else if (strncmp(buf, "GET", 3) == 0)
        {
            char filename[256];
            if (sscanf(buf, "%s %s", cmd, filename) != 2)
            {
                write(sock, "ERROR GET requires filename\n", 29);
                continue;
            }

            int fd = open(filename, O_RDONLY);
            if (fd < 0)
            {
                write(sock, "ERROR File not found\n", 21);
                continue;
            }

            struct stat st;
            fstat(fd, &st);
            long filesize = st.st_size;

            snprintf(buf, sizeof(buf), "%ld\n", filesize);
            write(sock, buf, strlen(buf));

            char filebuf[4096];
            ssize_t nread;
            while ((nread = read(fd, filebuf, sizeof(filebuf))) > 0)
            {
                if (write(sock, filebuf, nread) != nread)
                {
                    close(fd);
                    goto cleanup_get;
                }
            }
            close(fd);
        cleanup_get:
            continue;
        }
        else
        {
            write(sock, "ERROR Unknown command\n", 22);
        }
    }

    nclients--;
    printf("-disconnect\n");
    printusers();
    close(sock);
}

int main(int argc, char *argv[])
{
    int sockfd, newsockfd;
    int portno;
    pid_t pid;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;

    printf("TCP SERVER DEMO (Enhanced)\n");

    if (argc < 2)
    {
        fprintf(stderr, "ERROR, no port provided\n");
        exit(1);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

    bzero((char *)&serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");

    listen(sockfd, 5);
    clilen = sizeof(cli_addr);

    while (1)
    {
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0)
            error("ERROR on accept");
        nclients++;
        printusers();

        pid = fork();
        if (pid < 0)
            error("ERROR on fork");
        if (pid == 0)
        {
            close(sockfd);
            dostuff(newsockfd);
            exit(0);
        }
        else
            close(newsockfd);
    }
    close(sockfd);
    return 0;
}
