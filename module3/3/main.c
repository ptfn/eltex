#include <stdio.h>
#include "contact.h"

#define DB_FILE "phonebook.dat"

int main() {
    PhoneBook pb;
    init_phonebook(&pb);

    if (load_phonebook(&pb, DB_FILE) != 0) {
        printf("Ошибка загрузки данных из файла. Продолжаем с пустой книгой.\n");
    }

    int choice;
    int running = 1;
    while (running) {
        printf("\n--- Телефонная книга ---\n");
        printf("1. Добавить контакт\n");
        printf("2. Просмотреть все контакты\n");
        printf("3. Редактировать контакт\n");
        printf("4. Удалить контакт\n");
        printf("5. Поиск контактов\n");
        printf("6. Выход\n");
        printf("Выберите действие: ");
        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n');
            printf("Неверный ввод. Пожалуйста, введите число.\n");
            continue;
        }
        while (getchar() != '\n');

        switch (choice) {
            case 1: add_contact(&pb); break;
            case 2:
                list_contacts(&pb);
                if (pb.count > 0) {
                    printf("Введите ID контакта для подробного просмотра (0 для возврата): ");
                    int id;
                    if (scanf("%d", &id) == 1) {
                        while (getchar() != '\n');
                        if (id != 0) view_contact(&pb, id);
                    } else {
                        while (getchar() != '\n');
                    }
                }
                break;
            case 3: edit_contact(&pb); break;
            case 4: delete_contact(&pb); break;
            case 5: search_contacts(&pb); break;
            case 6:
                running = 0;
                break;
            default: printf("Неверный выбор. Попробуйте снова.\n");
        }
    }

    if (save_phonebook(&pb, DB_FILE) != 0) {
        printf("Ошибка сохранения данных в файл.\n");
    }

    free_phonebook(&pb);
    printf("Выход из программы.\n");
    return 0;
}
