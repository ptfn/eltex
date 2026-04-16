#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>
#include <errno.h>

#define SHM_NAME        "/shm_ex11"
#define SEM_DATA_NAME   "/sem_data_ex11"
#define SEM_RESULT_NAME "/sem_result_ex11"

#define MAX_NUMBERS 100

typedef struct {
    int count;
    int numbers[MAX_NUMBERS];
    int min;
    int max;
    int terminate;
} shared_data;

volatile sig_atomic_t stop = 0;

void sigint_handler(int sig) {
    stop = 1;
}

int main() {
    srand(time(NULL) ^ getpid());

    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(1);
    }
    if (ftruncate(shm_fd, sizeof(shared_data)) == -1) {
        perror("ftruncate");
        exit(1);
    }
    shared_data *shm = mmap(NULL, sizeof(shared_data),
                            PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    close(shm_fd);

    shm->count = 0;
    shm->terminate = 0;

    sem_t *sem_data = sem_open(SEM_DATA_NAME, O_CREAT, 0666, 0);
    if (sem_data == SEM_FAILED) {
        perror("sem_open data");
        exit(1);
    }
    sem_t *sem_result = sem_open(SEM_RESULT_NAME, O_CREAT, 0666, 0);
    if (sem_result == SEM_FAILED) {
        perror("sem_open result");
        exit(1);
    }

    shm_unlink(SHM_NAME);
    sem_unlink(SEM_DATA_NAME);
    sem_unlink(SEM_RESULT_NAME);

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(1);
    }

    if (pid == 0) {
        while (1) {
            if (sem_wait(sem_data) == -1) {
                perror("child sem_wait data");
                break;
            }

            if (shm->terminate)
                break;

            int count = shm->count;
            if (count > 0) {
                int min = shm->numbers[0];
                int max = shm->numbers[0];
                for (int i = 1; i < count; i++) {
                    if (shm->numbers[i] < min) min = shm->numbers[i];
                    if (shm->numbers[i] > max) max = shm->numbers[i];
                }
                shm->min = min;
                shm->max = max;
            } else {
                shm->min = shm->max = 0;
            }

            if (sem_post(sem_result) == -1) {
                perror("child sem_post result");
                break;
            }
        }

        munmap(shm, sizeof(shared_data));
        sem_close(sem_data);
        sem_close(sem_result);
        exit(0);
    } else {
        int set_count = 0;

        while (1) {
            int count = rand() % MAX_NUMBERS + 1;
            shm->count = count;
            for (int i = 0; i < count; i++) {
                shm->numbers[i] = rand() % 1000;  // числа в диапазоне 0–999
            }

            if (sem_post(sem_data) == -1) {
                perror("parent sem_post data");
                break;
            }

            int ret;
            do {
                ret = sem_wait(sem_result);
            } while (ret == -1 && errno == EINTR && !stop);

            if (ret == -1) {
                if (errno == EINTR && stop) {
                    shm->terminate = 1;
                    sem_post(sem_data);
                    break;
                } else {
                    perror("parent sem_wait result");
                    break;
                }
            }

            if (stop) {
                shm->terminate = 1;
                sem_post(sem_data);
                break;
            }

            set_count++;
            printf("Набір %d: min = %d, max = %d (кількість чисел: %d)\n",
                   set_count, shm->min, shm->max, shm->count);
        }

        waitpid(pid, NULL, 0);

        printf("Оброблено наборів даних: %d\n", set_count);

        munmap(shm, sizeof(shared_data));
        sem_close(sem_data);
        sem_close(sem_result);
    }

    return 0;
}
