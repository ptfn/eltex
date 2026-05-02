#include <dirent.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_COMMANDS 64

typedef struct {
    char *name;
    double (*func)(double, double);
    void *handle;
} Command;

int main(int argc, char *argv[]) {
    const char *lib_dir = (argc > 1) ? argv[1] : "./libs";
    
    Command commands[MAX_COMMANDS];
    int count = 0;

    printf("Загрузка плагинов из '%s'...\n", lib_dir);

    DIR *dir = opendir(lib_dir);
    if (!dir) {
        fprintf(stderr, "Ошибка: не удалось открыть каталог '%s'\n", lib_dir);
        return 1;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && count < MAX_COMMANDS) {
        if (entry->d_name[0] == '.') continue;
        char *ext = strrchr(entry->d_name, '.');
        if (!ext || strcmp(ext, ".so") != 0) continue;

        char path[512];
        snprintf(path, sizeof(path), "%s/%s", lib_dir, entry->d_name);

        void *handle = dlopen(path, RTLD_LAZY);
        if (!handle) {
            fprintf(stderr, "Ошибка загрузки '%s': %s\n", path, dlerror());
            continue;
        }

        const char* (*getName)(void) = dlsym(handle, "get_name");
        double (*execFunc)(double, double) = dlsym(handle, "execute");

        if (!getName || !execFunc) {
            fprintf(stderr, "Ошибка: '%s' не содержит get_name или execute\n", path);
            dlclose(handle);
            continue;
        }

        commands[count].name = strdup(getName());
        commands[count].func = execFunc;
        commands[count].handle = handle;
        printf("Загружен: %s\n", commands[count].name);
        count++;
    }
    closedir(dir);

    if (count == 0) {
        fprintf(stderr, "Не загружено ни одного плагина. Завершение.\n");
        return 1;
    }

    printf("Всего загружено операций: %d\n", count);

    int choice;
    double a, b, result;

    while (1) {
        printf("\n--- Калькулятор ---\n");
        for (int i = 0; i < count; i++) {
            printf("%d. %s\n", i + 1, commands[i].name);
        }
        printf("0. Выход\n");
        printf("Выберите действие: ");

if (scanf("%d", &choice) != 1) {
            printf("Ошибка ввода.\n");
            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF);
            continue;
        }

        if (choice == 0) break;

        if (choice < 1 || choice > count) {
            printf("Неверный выбор.\n");
            continue;
        }

        printf("Введите два числа: ");
        if (scanf("%lf %lf", &a, &b) != 2) {
            printf("Ошибка ввода.\n");
            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF);
            continue;
        }

        result = commands[choice - 1].func(a, b);
        
        if (result != result) {  // NaN check
            printf("Ошибка: некорректная операция (деление на 0?)\n");
        } else {
            printf("%.2lf %s %.2lf = %.2lf\n", a, commands[choice - 1].name, b, result);
        }
    }

    printf("Освобождение библиотек...\n");
    for (int i = 0; i < count; i++) {
        free(commands[i].name);
        dlclose(commands[i].handle);
    }

    return 0;
}