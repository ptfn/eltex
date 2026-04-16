#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <errno.h>

#define MAX_NUMBERS 100
#define SHM_KEY 0x1234
#define SEM_KEY 0x5678

typedef struct {
    int n;                  
    int numbers[MAX_NUMBERS];
    int min;
    int max;
    int stop_flag;         
} shared_data_t;

volatile sig_atomic_t running = 1;  
int shmid = -1;
int semid = -1;
shared_data_t *shared = NULL;

void sigint_handler(int sig) {
    (void)sig;
    running = 0;
}

void sem_op(int semid, int sem_num, int op) {
    struct sembuf sb = {sem_num, op, 0};
    if (semop(semid, &sb, 1) == -1) {
        if (errno == EINTR) {
            return;
        }
        perror("semop");
        exit(EXIT_FAILURE);
    }
}

void sem_wait(int semid, int sem_num) {
    sem_op(semid, sem_num, -1);
}

void sem_signal(int semid, int sem_num) {
    sem_op(semid, sem_num, 1);
}

int create_semaphores() {
    int semid = semget(SEM_KEY, 2, IPC_CREAT | 0666);
    if (semid == -1) {
        perror("semget");
        exit(EXIT_FAILURE);
    }
    union semun {
        int val;
        struct semid_ds *buf;
        unsigned short *array;
    } arg;
    arg.val = 0;
    if (semctl(semid, 0, SETVAL, arg) == -1 ||
        semctl(semid, 1, SETVAL, arg) == -1) {
        perror("semctl SETVAL");
        exit(EXIT_FAILURE);
    }
    return semid;
}

void cleanup() {
    if (shared != NULL) {
        shmdt(shared);
    }
    if (shmid != -1) {
        shmctl(shmid, IPC_RMID, NULL);
    }
    if (semid != -1) {
        semctl(semid, 0, IPC_RMID);
    }
}

int main() {
    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;  
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    srand(time(NULL) ^ getpid());

    shmid = shmget(SHM_KEY, sizeof(shared_data_t), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    shared = (shared_data_t *)shmat(shmid, NULL, 0);
    if (shared == (void *)-1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }
    shared->stop_flag = 0;

    semid = create_semaphores();

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        cleanup();
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        while (1) {
            sem_wait(semid, 0);
            
            if (shared->stop_flag) {
                break;
            }

            int n = shared->n;
            if (n <= 0 || n > MAX_NUMBERS) {
                fprintf(stderr, "Child: invalid n=%d\n", n);
                break;
            }

            int min = shared->numbers[0];
            int max = shared->numbers[0];
            for (int i = 1; i < n; i++) {
                if (shared->numbers[i] < min) min = shared->numbers[i];
                if (shared->numbers[i] > max) max = shared->numbers[i];
            }
            shared->min = min;
            shared->max = max;

            sem_signal(semid, 1);
        }
        shmdt(shared);
        exit(EXIT_SUCCESS);
    } else {
        int set_count = 0;
        while (running) {
            int n = rand() % MAX_NUMBERS + 1;  // от 1 до MAX_NUMBERS
            shared->n = n;
            for (int i = 0; i < n; i++) {
                shared->numbers[i] = rand() % 1000;  // числа 0..999
            }

            sem_signal(semid, 0);
            sem_wait(semid, 1);
            printf("Набор %d: количество чисел = %d, минимум = %d, максимум = %d\n",
                   set_count + 1, n, shared->min, shared->max);
            set_count++;
        }

        shared->stop_flag = 1;
        sem_signal(semid, 0);
        waitpid(pid, NULL, 0);
        printf("\nОбработано наборов данных: %d\n", set_count);

        cleanup();
    }

    return 0;
}
