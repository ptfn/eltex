#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>
#include <errno.h>

#define FILENAME "numbers.txt"
#define SEM_NAME "/prod_cons_sem"

sem_t *sem;

void parent_process() {
    int fd = open(FILENAME, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd == -1) {
        perror("open");
        exit(1);
    }

    srand(time(NULL) ^ (getpid() << 16));

    for (int i = 0; i < 10; i++) {
        int num_count = rand() % 10 + 1;
        char line[256];
        int pos = 0;
        for (int j = 0; j < num_count; j++) {
            pos += snprintf(line + pos, sizeof(line) - pos, "%d%c",
                            rand() % 1000, " \n"[j == num_count - 1]);
        }

        sem_wait(sem);
        write(fd, line, strlen(line));
        fsync(fd);
        sem_post(sem);

        printf("Parent wrote: %s", line);
        sleep(1);
    }

    const char *end_marker = "END\n";
    sem_wait(sem);
    write(fd, end_marker, strlen(end_marker));
    fsync(fd);
    sem_post(sem);

    close(fd);
}

void child_process() {
    int fd = open(FILENAME, O_RDONLY);
    if (fd == -1) {
        perror("open");
        exit(1);
    }

    off_t offset = 0;
    int done = 0;

    while (!done) {
        sem_wait(sem);

        off_t end = lseek(fd, 0, SEEK_END);
        if (end > offset) {
            char buf[4096];
            lseek(fd, offset, SEEK_SET);
            ssize_t n = read(fd, buf, sizeof(buf) - 1);
            if (n > 0) {
                buf[n] = '\0';
                offset = end;

                char *line = buf;
                while (line && *line) {
                    char *nl = strchr(line, '\n');
                    if (nl) *nl = '\0';

                    long min = 999999, max = -1;
                    char *p = line;
                    while (*p) {
                        while (*p == ' ') p++;
                        if (*p == '\0') break;
                        char *endp;
                        long val = strtol(p, &endp, 10);
                        if (endp != p) {
                            if (val < min) min = val;
                            if (val > max) max = val;
                            p = endp;
                        } else {
                            p++;
                        }
                    }

                    if (strcmp(line, "END") == 0) {
                        done = 1;
                        line = nl ? nl + 1 : NULL;
                        break;
                    }

                    if (max >= 0) {
                        printf("Child line: %s\n", line);
                        printf("  Min: %ld, Max: %ld\n", min, max);
                    }

                    line = nl ? nl + 1 : NULL;
                }
            }
        }

        sem_post(sem);
        usleep(500000);
    }

    close(fd);
}

int main() {
    sem_unlink(SEM_NAME);
    sem = sem_open(SEM_NAME, O_CREAT | O_EXCL, 0666, 1);
    if (sem == SEM_FAILED) {
        perror("sem_open");
        return 1;
    }

    setbuf(stdout, NULL);

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        child_process();
    } else {
        parent_process();
        wait(NULL);
        sem_unlink(SEM_NAME);
    }

    sem_close(sem);
    return 0;
}
