#include <dirent.h>
#include <dlfcn.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_COMMANDS 64
#define LIB_DIR "./libs"

typedef struct {
    char *name;
    double (*func)(double, double);
    void *handle;
} Command;

int main() {
    Command commands[MAX_COMMANDS];
    int count = 0;

    DIR *dir = opendir(LIB_DIR);
    if (!dir) {
        fprintf(stderr, "Ошибка: не удалось открыть каталог '%s'\n", LIB_DIR);
        return 1;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && count < MAX_COMMANDS) {
        if (!strstr(entry->d_name, ".so")) continue;

        char path[512];
        snprintf(path, sizeof(path), "%s/%s", LIB_DIR, entry->d_name);

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
        count++;
    }
    closedir(dir);

    if (count == 0) {
        fprintf(stderr, "Не загружено ни одного плагина. Завершение.\n");
        return 1;
    }

    int choice;
    double a, b, result;

    while (1) {
        printf("\n--- Калькулятор (динамический) ---\n");
        for (int i = 0; i < count; i++) {
            printf("%d. %s\n", i + 1, commands[i].name);
        }
        printf("0. Выход\n");
        printf("Выберите действие: ");

        if (scanf("%d", &choice) != 1) {
            printf("Ошибка ввода. Попробуйте снова.\n");
            while (getchar() != '\n');
            continue;
        }

        if (choice == 0) {
            printf("До свидания!\n");
            break;
        }

        if (choice < 1 || choice > count) {
            printf("Неверный выбор. Попробуйте снова.\n");
            continue;
        }

        printf("Введите два числа: ");
        if (scanf("%lf %lf", &a, &b) != 2) {
            printf("Ошибка ввода. Попробуйте снова.\n");
            while (getchar() != '\n');
            continue;
        }

        result = commands[choice - 1].func(a, b);

        if (isnan(result)) {
            printf("Ошибка: некорректная операция!\n");
        } else {
            printf("Результат: %.2lf\n", result);
        }
    }

    for (int i = 0; i < count; i++) {
        free(commands[i].name);
        dlclose(commands[i].handle);
    }

    return 0;
}
