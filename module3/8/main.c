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

#define SEM_KEY 1234
#define SEM_PERMS 0666

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

int create_semaphore(key_t key, int init_value) {
    int semid = semget(key, 1, IPC_CREAT | SEM_PERMS);
    if (semid == -1) {
        perror("semget");
        exit(EXIT_FAILURE);
    }
    
    union semun arg;
    arg.val = init_value;
    if (semctl(semid, 0, SETVAL, arg) == -1) {
        perror("semctl");
        exit(EXIT_FAILURE);
    }
    
    return semid;
}

void wait_semaphore(int semid) {
    struct sembuf op = {0, -1, 0};
    if (semop(semid, &op, 1) == -1) {
        perror("semop wait");
        exit(EXIT_FAILURE);
    }
}

void signal_semaphore(int semid) {
    struct sembuf op = {0, 1, 0};
    if (semop(semid, &op, 1) == -1) {
        perror("semop signal");
        exit(EXIT_FAILURE);
    }
}

void producer(const char *filename, int num_products) {
    key_t file_key = ftok(filename, 1);
    int semid = create_semaphore(file_key, 1);
    
    int fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0666);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }
    
    srand(time(NULL) + getpid());
    
    for (int i = 0; i < num_products; i++) {
        wait_semaphore(semid);
        
        char line[256];
        int num = rand() % 1000;
        snprintf(line, sizeof(line), "Producer %d: Line %d with number %d\n", getpid(), i, num);
        
        write(fd, line, strlen(line));
        printf("Producer %d wrote: %s", getpid(), line);
        
        signal_semaphore(semid);
        
        usleep(rand() % 100000);
    }
    
    close(fd);
    semctl(semid, 0, IPC_RMID);
}

void consumer(const char *filename, int num_consumes) {
    key_t file_key = ftok(filename, 1);
    int semid = semget(file_key, 1, 0666);
    if (semid == -1) {
        perror("semget");
        exit(EXIT_FAILURE);
    }
    
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }
    
    for (int i = 0; i < num_consumes; i++) {
        wait_semaphore(semid);
        
        char line[256];
        lseek(fd, 0, SEEK_SET);
        
        printf("\nConsumer %d analysis:\n", getpid());
        while (read(fd, line, sizeof(line) - 1) > 0) {
            line[sizeof(line) - 1] = '\0';
            
            char *token = strtok(line, " ");
            int min = 1000, max = -1;
            int found = 0;
            
            while (token != NULL) {
                char *endptr;
                long num = strtol(token, &endptr, 10);
                
                if (*endptr == '\0' || *endptr == '\n') {
                    found = 1;
                    if (num < min) min = num;
                    if (num > max) max = num;
                }
                
                token = strtok(NULL, " ");
            }
            
            if (found) {
                printf("Line: %s", line);
                printf("  Min: %d, Max: %d\n", min, max);
            }
        }
        
        signal_semaphore(semid);
        usleep(rand() % 200000);
    }
    
    close(fd);
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        printf("Usage: %s <filename> <num_producers> <num_consumers> [products_per_producer] [consumes_per_consumer]\n", argv[0]);
        return 1;
    }
    
    const char *filename = argv[1];
    int num_producers = atoi(argv[2]);
    int num_consumers = atoi(argv[3]);
    int products_per_producer = (argc > 4) ? atoi(argv[4]) : 5;
    int consumes_per_consumer = (argc > 5) ? atoi(argv[5]) : 3;
    
    printf("Starting producers and consumers...\n");
    printf("File: %s\n", filename);
    printf("Producers: %d, Products per producer: %d\n", num_producers, products_per_producer);
    printf("Consumers: %d, Consumes per consumer: %d\n\n", num_consumers, consumes_per_consumer);
    
    pid_t pids[num_producers + num_consumers];
    
    for (int i = 0; i < num_producers; i++) {
        pids[i] = fork();
        if (pids[i] == 0) {
            producer(filename, products_per_producer);
            exit(0);
        } else if (pids[i] < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
    }
    
    for (int i = 0; i < num_consumers; i++) {
        pids[num_producers + i] = fork();
        if (pids[num_producers + i] == 0) {
            consumer(filename, consumes_per_consumer);
            exit(0);
        } else if (pids[num_producers + i] < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
    }
    
    for (int i = 0; i < num_producers + num_consumers; i++) {
        waitpid(pids[i], NULL, 0);
    }
    
    printf("\nAll processes completed.\n");
    return 0;
}
