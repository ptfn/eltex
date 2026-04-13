#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "calculator.h"

typedef struct {
  char *name;
  double (*func)(double, double);
} Command;

void printMenu(Command *commands, int count) {
  printf("\n--- Калькулятор (динамический) ---\n");

  for (int i = 0; i < count; i++) {
    printf("%d. %s\n", i + 1, commands[i].name);
  }

  printf("0. Выход\n");
  printf("Выберите действие: ");
}

int main() {
  Command commands[] = {{"Сложение", add},
                        {"Вычитание", subtract},
                        {"Умножение", multiply},
                        {"Деление", divide},
  			{"Максимум", divide}};
  int commandCount = sizeof(commands) / sizeof(commands[0]);

  int choice;
  double a, b, result;

  while (1) {
    printMenu(commands, commandCount);
    scanf("%d", &choice);

    if (choice == 0) {
      printf("До свидания!\n");
      break;
    }

    if (choice < 1 || choice > commandCount) {
      printf("Неверный выбор. Попробуйте снова.\n");
      continue;
    }

    printf("Введите два числа: ");
    scanf("%lf %lf", &a, &b);

    result = commands[choice - 1].func(a, b);

    if (commands[choice - 1].func == divide && b == 0) {
      printf("Ошибка: деление на ноль!\n");
    } else {
      printf("Результат: %.2lf\n", result);
    }
  }

  return 0;
}
