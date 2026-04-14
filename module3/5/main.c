#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <string.h>

int counter = 0;
int signal_count = 0;
FILE *output_file;
time_t last_signal_time;

void handle_sigint(int sig) {
    time_t current_time;
    time(&current_time);
    
    fprintf(output_file, "Получен сигнал SIGINT в %s", ctime(&current_time));
    fprintf(output_file, "Значение счетчика на момент получения сигнала: %d\n", counter);
    signal_count++;
    
    if (signal_count >= 3) {
        fprintf(output_file, "Получен третий сигнал SIGINT. Завершение программы.\n");
        fclose(output_file);
        exit(0);
    }
}

int main() {
    output_file = fopen("1", "w");
    if (output_file == NULL) {
        perror("Ошибка открытия файла");
        return 1;
    }
    
    signal(SIGINT, handle_sigint);
    
    fprintf(output_file, "Программа запущена. Начинаем запись счетчика...\n");
    
    while (1) {
        counter++;
        time_t current_time;
        time(&current_time);
        
        fprintf(output_file, "%d %s", counter, ctime(&current_time));
        fflush(output_file);
        
        sleep(1);
    }
    
    fclose(output_file);
    return 0;
}
