#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

int semid;

void sem_wait() {
    struct sembuf op = {0, -1, 0};
    if (semop(semid, &op, 1) == -1) {
        perror("semop wait");
        _exit(1);
    }
}

void sem_signal() {
    struct sembuf op = {0, 1, 0};
    if (semop(semid, &op, 1) == -1) {
        perror("semop signal");
        _exit(1);
    }
}

void producer(const char *filename, int count) {
    int fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0666);
    if (fd == -1) {
        perror("open");
        exit(1);
    }

    srand(time(NULL) ^ (getpid() << 16));

    for (int i = 0; i < count; i++) {
        int num_count = rand() % 10 + 1;
        char line[256];
        int pos = 0;
        for (int j = 0; j < num_count; j++) {
            pos += snprintf(line + pos, sizeof(line) - pos, "%d%c", rand() % 1000, " \n"[j == num_count - 1]);
        }

        sem_wait();
        write(fd, line, strlen(line));
        fsync(fd);
        sem_signal();

        usleep(rand() % 100000);
    }

    close(fd);
}

void consumer(const char *filename, int times) {
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("open");
        exit(1);
    }

    off_t offset = 0;

    for (int i = 0; i < times; i++) {
        sem_wait();

        off_t end = lseek(fd, 0, SEEK_END);
        if (end <= offset) {
            sem_signal();
            usleep(100000);
            continue;
        }

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
                    char *end;
                    long val = strtol(p, &end, 10);
                    if (end != p) {
                        if (val < min) min = val;
                        if (val > max) max = val;
                        p = end;
                    } else {
                        p++;
                    }
                }

                if (max >= 0) {
                    printf("Consumer %d line: %s\n", getpid(), line);
                    printf("  Min: %ld, Max: %ld\n", min, max);
                }

                line = nl ? nl + 1 : NULL;
            }
        }

        sem_signal();
        usleep(rand() % 200000);
    }

    close(fd);
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        printf("Usage: %s <filename> <num_producers> <num_consumers> [products] [consumes]\n", argv[0]);
        return 1;
    }

    const char *filename = argv[1];
    int np = atoi(argv[2]);
    int nc = atoi(argv[3]);
    int products = argc > 4 ? atoi(argv[4]) : 5;
    int consumes = argc > 5 ? atoi(argv[5]) : 5;

    key_t key = ftok(filename, 1);
    semid = semget(key, 1, IPC_CREAT | 0666);
    if (semid == -1) {
        perror("semget");
        return 1;
    }

    union semun arg;
    arg.val = 1;
    semctl(semid, 0, SETVAL, arg);

    setbuf(stdout, NULL);
    printf("File: %s | Producers: %d (%d each) | Consumers: %d (%d each)\n",
           filename, np, products, nc, consumes);

    pid_t pids[np + nc];
    int total = np + nc;

    for (int i = 0; i < np; i++) {
        pids[i] = fork();
        if (pids[i] == 0) { producer(filename, products); exit(0); }
    }

    for (int i = 0; i < nc; i++) {
        pids[np + i] = fork();
        if (pids[np + i] == 0) { consumer(filename, consumes); exit(0); }
    }

    for (int i = 0; i < total; i++) waitpid(pids[i], NULL, 0);

    semctl(semid, 0, IPC_RMID);
    printf("Done.\n");
    return 0;
}
