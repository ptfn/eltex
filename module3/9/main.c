#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>
#include <errno.h>

#define FILENAME "numbers.txt"
#define SEM_NAME "/my_semaphore"
#define MAX_NUMBERS 10
#define MAX_VALUE 100

void parent_process(sem_t *sem) {
    printf("Родительский процесс: генерация случайных чисел...\n");
    
    int fd = open(FILENAME, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd == -1) {
        perror("Ошибка открытия файла");
        exit(EXIT_FAILURE);
    }
    
    srand(time(NULL));
    
    for (int i = 0; i < MAX_NUMBERS; i++) {
        int num = rand() % MAX_VALUE;
        
        sem_wait(sem);
        
        char buffer[32];
        int len = snprintf(buffer, sizeof(buffer), "%d\n", num);
        write(fd, buffer, len);
        
        printf("Родитель: записано число %d\n", num);
      
        sem_post(sem);
        
        sleep(1);
    }
    
    close(fd);
    printf("Родительский процесс завершил запись чисел.\n");
}

void child_process(sem_t *sem) {
    printf("Дочерний процесс: анализ чисел...\n");
    
    int fd = open(FILENAME, O_RDONLY);
    if (fd == -1) {
        perror("Ошибка открытия файла");
        exit(EXIT_FAILURE);
    }
    
    char buffer[32];
    ssize_t bytes_read;
    
    while ((bytes_read = read(fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';
        
        sem_wait(sem);
        
        char *token = strtok(buffer, "\n");
        while (token != NULL) {
            int num = atoi(token);
            if (num != 0) {
                static int min = MAX_VALUE;
                static int max = 0;
                static int first_num = 1;
                
                if (first_num) {
                    min = num;
                    max = num;
                    first_num = 0;
                } else {
                    if (num < min) min = num;
                    if (num > max) max = num;
                }
                
                printf("Дочерний: обработано число %d (текущий min: %d, max: %d)\n", 
                       num, min, max);
            }
            token = strtok(NULL, "\n");
        }
        
        sem_post(sem);
        
        sleep(1);
    }
    
    close(fd);
    printf("Дочерний процесс завершил анализ.\n");
}

int main() {
    sem_t *sem = sem_open(SEM_NAME, O_CREAT | O_EXCL, 0666, 1);
    if (sem == SEM_FAILED) {
        if (errno == EEXIST) {
            sem = sem_open(SEM_NAME, 0);
            if (sem == SEM_FAILED) {
                perror("Ошибка открытия семафора");
                exit(EXIT_FAILURE);
            }
            printf("Семафор уже существовал, продолжаем работу.\n");
        } else {
            perror("Ошибка создания семафора");
            exit(EXIT_FAILURE);
        }
    } else {
        printf("Семафор успешно создан.\n");
    }
    
    pid_t pid = fork();
    
    if (pid == -1) {
        perror("Ошибка fork");
        exit(EXIT_FAILURE);
    }
    
    if (pid == 0) {
        child_process(sem);
    } else {
        parent_process(sem);
        
        wait(NULL);
        
        if (sem_unlink(SEM_NAME) == -1) {
            perror("Ошибка удаления семафора");
        } else {
            printf("Семафор успешно удален.\n");
        }
    }
    
    if (sem_close(sem) == -1) {
        perror("Ошибка закрытия семафора");
        exit(EXIT_FAILURE);
    }
    
    return 0;
}
