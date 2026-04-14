#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <math.h>

int isNumeric(const char *str) {
    char *endptr;
    strtod(str, &endptr);
    return (endptr != str && *endptr == '\0');
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <arg1> <arg2> ...\n", argv[0]);
        return 1;
    }

    int n = argc - 1;
    int half = n / 2;
    
    pid_t pid = fork();
    
    if (pid == 0) {
        for (int i = half + 1; i < argc; i++) {
            if (isNumeric(argv[i])) {
                double num = strtod(argv[i], NULL);
                printf("%s: %f, %f\n", argv[i], num, num * 2);
            } else {
                printf("%s\n", argv[i]);
            }
        }
        exit(0);
    } else if (pid > 0) {
        for (int i = 1; i <= half; i++) {
            if (isNumeric(argv[i])) {
                double num = strtod(argv[i], NULL);
                printf("%s: %f, %f\n", argv[i], num, num * 2);
            } else {
                printf("%s\n", argv[i]);
            }
        }
        
        wait(NULL);
    } else {
        perror("Fork failed");
        return 1;
    }

    return 0;
}
