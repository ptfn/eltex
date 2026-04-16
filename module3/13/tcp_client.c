#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/stat.h>

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int readbytes(int sock, char *buf, int len)
{
    int total = 0;
    while (total < len)
    {
        int n = recv(sock, buf + total, len - total, 0);
        if (n <= 0)
            return -1;
        total += n;
    }
    return total;
}

void do_put(int sock, const char *filename)
{
    char buf[1024];
    FILE *f = fopen(filename, "rb");
    if (!f)
    {
        printf("Cannot open local file %s\n", filename);
        return;
    }

    fseek(f, 0, SEEK_END);
    long filesize = ftell(f);
    rewind(f);

    snprintf(buf, sizeof(buf), "PUT %s\n", filename);
    send(sock, buf, strlen(buf), 0);

    snprintf(buf, sizeof(buf), "%ld\n", filesize);
    send(sock, buf, strlen(buf), 0);

    char filebuf[4096];
    size_t nread;
    while ((nread = fread(filebuf, 1, sizeof(filebuf), f)) > 0)
    {
        if (send(sock, filebuf, nread, 0) != nread)
        {
            printf("Error sending file data\n");
            fclose(f);
            return;
        }
    }
    fclose(f);

    int n = recv(sock, buf, sizeof(buf) - 1, 0);
    if (n > 0)
    {
        buf[n] = '\0';
        printf("Server response: %s", buf);
    }
}

void do_get(int sock, const char *filename)
{
    char buf[1024];
    snprintf(buf, sizeof(buf), "GET %s\n", filename);
    send(sock, buf, strlen(buf), 0);

    int n = recv(sock, buf, sizeof(buf) - 1, 0);
    if (n <= 0)
    {
        printf("Server closed connection\n");
        return;
    }
    buf[n] = '\0';

    if (strncmp(buf, "ERROR", 5) == 0)
    {
        printf("Server error: %s", buf);
        return;
    }

    long filesize = atol(buf);
    if (filesize <= 0)
    {
        printf("Invalid file size received\n");
        return;
    }

    char localname[300];
    snprintf(localname, sizeof(localname), "downloaded_%s", filename);
    FILE *f = fopen(localname, "wb");
    if (!f)
    {
        printf("Cannot create local file %s\n", localname);
        char dummy[4096];
        long remain = filesize;
        while (remain > 0)
        {
            int toread = (remain < sizeof(dummy)) ? remain : sizeof(dummy);
            if (readbytes(sock, dummy, toread) <= 0)
                break;
            remain -= toread;
        }
        return;
    }

    char filebuf[4096];
    long remain = filesize;
    while (remain > 0)
    {
        int toread = (remain < sizeof(filebuf)) ? remain : sizeof(filebuf);
        int rc = readbytes(sock, filebuf, toread);
        if (rc <= 0)
        {
            printf("Error receiving file data\n");
            fclose(f);
            return;
        }
        fwrite(filebuf, 1, rc, f);
        remain -= rc;
    }
    fclose(f);
    printf("File downloaded as %s\n", localname);
}

int main(int argc, char *argv[])
{
    int my_sock, portno;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char buf[1024];
    int n;

    printf("TCP DEMO CLIENT (Enhanced)\n");

    if (argc < 3)
    {
        fprintf(stderr, "usage %s hostname port\n", argv[0]);
        exit(0);
    }

    portno = atoi(argv[2]);
    my_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (my_sock < 0)
        error("ERROR opening socket");

    server = gethostbyname(argv[1]);
    if (server == NULL)
    {
        fprintf(stderr, "ERROR, no such host\n");
        exit(0);
    }

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
          (char *)&serv_addr.sin_addr.s_addr,
          server->h_length);
    serv_addr.sin_port = htons(portno);

    if (connect(my_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR connecting");

    n = recv(my_sock, buf, sizeof(buf) - 1, MSG_DONTWAIT);
    if (n > 0)
    {
        buf[n] = '\0';
        printf("%s", buf);
    }

    printf("\nAvailable commands:\n");
    printf("  CALC <op> <a> <b>   (op: + - * /)\n");
    printf("  PUT <filename>      (upload file to server)\n");
    printf("  GET <filename>      (download file from server)\n");
    printf("  QUIT                (exit)\n\n");

    while (1)
    {
        printf("> ");
        fflush(stdout);
        if (!fgets(buf, sizeof(buf), stdin))
            break;

        buf[strcspn(buf, "\n")] = '\0';

        if (strlen(buf) == 0)
            continue;

        if (strcmp(buf, "QUIT") == 0)
        {
            send(my_sock, "QUIT\n", 5, 0);
            break;
        }
        else if (strncmp(buf, "PUT ", 4) == 0)
        {
            char *filename = buf + 4;
            while (*filename == ' ') filename++;
            do_put(my_sock, filename);
        }
        else if (strncmp(buf, "GET ", 4) == 0)
        {
            char *filename = buf + 4;
            while (*filename == ' ') filename++;
            do_get(my_sock, filename);
        }
        else if (strncmp(buf, "CALC", 4) == 0)
        {
            strcat(buf, "\n");
            send(my_sock, buf, strlen(buf), 0);

            n = recv(my_sock, buf, sizeof(buf) - 1, 0);
            if (n > 0)
            {
                buf[n] = '\0';
                printf("Result: %s", buf);
            }
        }
        else
        {
            printf("Unknown command. Try CALC, PUT, GET or QUIT.\n");
        }
    }

    close(my_sock);
    return 0;
}
