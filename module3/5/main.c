#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>

int counter = 0;
int signal_count = 0;
int output_fd;

void handle_signal(int sig) {
    const char *msg;
    if (sig == SIGINT) {
        msg = "Получен и обработан сигнал SIGINT\n";
        write(output_fd, msg, strlen(msg));
        signal_count++;

        if (signal_count >= 3) {
            msg = "Получен третий сигнал SIGINT. Завершение программы.\n";
            write(output_fd, msg, strlen(msg));
            close(output_fd);
            _exit(0);
        }
    } else if (sig == SIGQUIT) {
        msg = "Получен и обработан сигнал SIGQUIT\n";
        write(output_fd, msg, strlen(msg));
    }
}

int main() {
    output_fd = open("output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (output_fd == -1) {
        perror("open");
        return 1;
    }

    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL);

    const char *intro = "Программа запущена. Начинаем запись счетчика...\n";
    write(output_fd, intro, strlen(intro));

    while (1) {
        counter++;
        time_t t;
        time(&t);
        char *time_str = ctime(&t);
        char buf[64];
        int n = snprintf(buf, sizeof(buf), "%d %s", counter, time_str);
        write(output_fd, buf, n);
        sleep(1);
    }

    close(output_fd);
    return 0;
}
