#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

#define MAX_LINE 1024
#define MAX_ARGS 64

void print_greeting() {
    printf("Добро пожаловать в мой Shell! Введите команду (или 'exit' для выхода):\n");
}

int main() {
    char line[MAX_LINE];
    char *args[MAX_ARGS];
    char *command;
    int status;
    pid_t pid;

    print_greeting();

    while (1) {
        printf("> ");
        fflush(stdout);

        if (fgets(line, MAX_LINE, stdin) == NULL) {
            break; // Обработка Ctrl+D
        }

        line[strcspn(line, "\n")] = '\0';

        if (line[0] == '\0') {
            continue;
        }

        int i = 0;
        command = strtok(line, " ");
        while (command != NULL && i < MAX_ARGS - 1) {
            args[i++] = command;
            command = strtok(NULL, " ");
        }
        args[i] = NULL;

        if (strcmp(args[0], "exit") == 0) {
            break;
        }

        if (strcmp(args[0], "cd") == 0) {
            if (args[1] == NULL) {
                chdir(getenv("HOME"));
            } else {
                if (chdir(args[1]) != 0) {
                    perror("cd: ошибка");
                }
            }
            continue;
        }

        pid = fork();
        if (pid == 0) {
            if (execvp(args[0], args) == -1) {
                fprintf(stderr, "Ошибка: команда '%s' не найдена\n", args[0]);
                exit(EXIT_FAILURE);
            }
        } else if (pid > 0) {
            waitpid(pid, &status, 0);
        } else {
            perror("Ошибка fork");
        }

        print_greeting();
    }

    printf("До свидания!\n");
    return 0;
}
