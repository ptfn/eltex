#include <stdio.h>
#include <stdlib.h>

#include "calculator.h"

int main() {
  int choice;
  double a, b, result;

  while (1) {
    printf("\n--- Калькулятор ---\n");
    printf("1. Сложение\n");
    printf("2. Вычитание\n");
    printf("3. Умножение\n");
    printf("4. Деление\n");
    printf("0. Выход\n");
    printf("Выберите действие: ");
    scanf("%d", &choice);

    if (choice == 0) {
      printf("До свидания!\n");
      break;
    }

    printf("Введите два числа: ");
    scanf("%lf %lf", &a, &b);

    switch (choice) {
      case 1:
        result = add(a, b);
        printf("Результат: %.2lf\n", result);
        break;
      case 2:
        result = subtract(a, b);
        printf("Результат: %.2lf\n", result);
        break;
      case 3:
        result = multiply(a, b);
        printf("Результат: %.2lf\n", result);
        break;
      case 4:
        result = divide(a, b);
        if (b == 0) {
          printf("Ошибка: деление на ноль!\n");
        } else {
          printf("Результат: %.2lf\n", result);
        }
        break;
      default:
        printf("Неверный выбор. Попробуйте снова.\n");
    }
  }

  return 0;
}
