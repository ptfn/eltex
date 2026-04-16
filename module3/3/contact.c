#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include "contact.h"

static void input_string(const char *prompt, char *buffer, int size, int required) {
    while (1) {
        printf("%s", prompt);
        if (fgets(buffer, size, stdin) != NULL) {
            size_t len = strlen(buffer);
            if (len > 0 && buffer[len-1] == '\n')
                buffer[len-1] = '\0';
            else {
                int ch;
                while ((ch = getchar()) != '\n' && ch != EOF);
            }
            if (required && strlen(buffer) == 0) {
                printf("Это поле обязательно. Пожалуйста, введите значение.\n");
                continue;
            }
            break;
        }
    }
}

static int find_index_by_id(PhoneBook *pb, int id) {
    for (int i = 0; i < pb->count; i++) {
        if (pb->contacts[i].id == id)
            return i;
    }
    return -1;
}

static int get_contact_id(PhoneBook *pb) {
    int id;
    printf("Введите ID контакта: ");
    if (scanf("%d", &id) != 1) {
        while (getchar() != '\n');
        return -1;
    }
    while (getchar() != '\n');
    if (find_index_by_id(pb, id) == -1)
        return -1;
    return id;
}

void init_phonebook(PhoneBook *pb) {
    pb->contacts = NULL;
    pb->count = 0;
    pb->next_id = 1;
}

void free_phonebook(PhoneBook *pb) {
    free(pb->contacts);
    pb->contacts = NULL;
    pb->count = 0;
    pb->next_id = 1;
}

int load_phonebook(PhoneBook *pb, const char *filename) {
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        return 0;
    }

    int header[2];
    ssize_t bytes_read = read(fd, header, sizeof(header));
    if (bytes_read != sizeof(header)) {
        close(fd);
        return -1;
    }
    int count = header[0];
    int next_id = header[1];

    if (count < 0) {
        close(fd);
        return -1;
    }

    Contact *temp = NULL;
    if (count > 0) {
        temp = malloc(count * sizeof(Contact));
        if (!temp) {
            close(fd);
            return -1;
        }
        bytes_read = read(fd, temp, count * sizeof(Contact));
        if (bytes_read != count * sizeof(Contact)) {
            free(temp);
            close(fd);
            return -1;
        }
    }

    free(pb->contacts);
    pb->contacts = temp;
    pb->count = count;
    pb->next_id = next_id;

    close(fd);
    return 0;
}

int save_phonebook(PhoneBook *pb, const char *filename) {
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open");
        return -1;
    }

    int header[2] = { pb->count, pb->next_id };
    ssize_t bytes_written = write(fd, header, sizeof(header));
    if (bytes_written != sizeof(header)) {
        close(fd);
        return -1;
    }

    if (pb->count > 0 && pb->contacts != NULL) {
        bytes_written = write(fd, pb->contacts, pb->count * sizeof(Contact));
        if (bytes_written != pb->count * sizeof(Contact)) {
            close(fd);
            return -1;
        }
    }

    close(fd);
    return 0;
}

void add_contact(PhoneBook *pb) {
    Contact new_contact;
    new_contact.id = pb->next_id++;

    printf("Добавление нового контакта (ID = %d):\n", new_contact.id);
    input_string("Фамилия (обязательно): ", new_contact.surname, NAME_LEN, 1);
    input_string("Имя (обязательно): ", new_contact.name, NAME_LEN, 1);
    input_string("Отчество (необязательно): ", new_contact.patronymic, NAME_LEN, 0);
    input_string("Место работы: ", new_contact.work, TEXT_LEN, 0);
    input_string("Должность: ", new_contact.position, TEXT_LEN, 0);
    input_string("Номера телефонов (можно несколько через запятую): ", new_contact.phones, MULTI_LEN, 0);
    input_string("Адреса email (можно несколько через запятую): ", new_contact.emails, MULTI_LEN, 0);
    input_string("Ссылки на соцсети: ", new_contact.social, MULTI_LEN, 0);
    input_string("Профили в мессенджерах: ", new_contact.messengers, MULTI_LEN, 0);

    Contact *temp = realloc(pb->contacts, (pb->count + 1) * sizeof(Contact));
    if (!temp) {
        printf("Ошибка выделения памяти. Контакт не добавлен.\n");
        pb->next_id--;
        return;
    }
    pb->contacts = temp;
    pb->contacts[pb->count] = new_contact;
    pb->count++;
    printf("Контакт успешно добавлен. ID: %d\n", new_contact.id);
}

void list_contacts(PhoneBook *pb) {
    if (pb->count == 0) {
        printf("Телефонная книга пуста.\n");
        return;
    }
    printf("Список контактов:\n");
    for (int i = 0; i < pb->count; i++) {
        printf("ID: %d | %s %s %s\n", 
               pb->contacts[i].id,
               pb->contacts[i].surname, 
               pb->contacts[i].name, 
               pb->contacts[i].patronymic);
    }
}

void view_contact(PhoneBook *pb, int id) {
    int idx = find_index_by_id(pb, id);
    if (idx == -1) {
        printf("Контакт с ID %d не найден.\n", id);
        return;
    }
    Contact *c = &pb->contacts[idx];
    printf("Контакт ID: %d\n", c->id);
    printf("Фамилия: %s\n", c->surname);
    printf("Имя: %s\n", c->name);
    printf("Отчество: %s\n", c->patronymic);
    printf("Место работы: %s\n", c->work);
    printf("Должность: %s\n", c->position);
    printf("Телефоны: %s\n", c->phones);
    printf("Email: %s\n", c->emails);
    printf("Соцсети: %s\n", c->social);
    printf("Мессенджеры: %s\n", c->messengers);
}

void edit_contact(PhoneBook *pb) {
    if (pb->count == 0) {
        printf("Нет контактов для редактирования.\n");
        return;
    }
    list_contacts(pb);
    int id = get_contact_id(pb);
    if (id == -1) {
        printf("Неверный ID контакта.\n");
        return;
    }
    int idx = find_index_by_id(pb, id);
    Contact *c = &pb->contacts[idx];
    printf("Редактирование контакта ID %d. Оставьте поле пустым, чтобы сохранить текущее значение.\n", id);

    char buffer[NAME_LEN];
    printf("Текущая фамилия: %s\n", c->surname);
    input_string("Новая фамилия (обязательно): ", buffer, NAME_LEN, 0);
    if (strlen(buffer) > 0) strcpy(c->surname, buffer);

    printf("Текущее имя: %s\n", c->name);
    input_string("Новое имя (обязательно): ", buffer, NAME_LEN, 0);
    if (strlen(buffer) > 0) strcpy(c->name, buffer);

    printf("Текущее отчество: %s\n", c->patronymic);
    input_string("Новое отчество: ", buffer, NAME_LEN, 0);
    if (strlen(buffer) > 0) strcpy(c->patronymic, buffer);

    printf("Текущее место работы: %s\n", c->work);
    input_string("Новое место работы: ", buffer, TEXT_LEN, 0);
    if (strlen(buffer) > 0) strcpy(c->work, buffer);

    printf("Текущая должность: %s\n", c->position);
    input_string("Новая должность: ", buffer, TEXT_LEN, 0);
    if (strlen(buffer) > 0) strcpy(c->position, buffer);

    printf("Текущие телефоны: %s\n", c->phones);
    input_string("Новые телефоны: ", buffer, MULTI_LEN, 0);
    if (strlen(buffer) > 0) strcpy(c->phones, buffer);

    printf("Текущие email: %s\n", c->emails);
    input_string("Новые email: ", buffer, MULTI_LEN, 0);
    if (strlen(buffer) > 0) strcpy(c->emails, buffer);

    printf("Текущие соцсети: %s\n", c->social);
    input_string("Новые соцсети: ", buffer, MULTI_LEN, 0);
    if (strlen(buffer) > 0) strcpy(c->social, buffer);

    printf("Текущие мессенджеры: %s\n", c->messengers);
    input_string("Новые мессенджеры: ", buffer, MULTI_LEN, 0);
    if (strlen(buffer) > 0) strcpy(c->messengers, buffer);

    printf("Контакт обновлён.\n");
}

void delete_contact(PhoneBook *pb) {
    if (pb->count == 0) {
        printf("Нет контактов для удаления.\n");
        return;
    }
    list_contacts(pb);
    int id = get_contact_id(pb);
    if (id == -1) {
        printf("Неверный ID контакта.\n");
        return;
    }
    int idx = find_index_by_id(pb, id);
    printf("Вы уверены, что хотите удалить контакт с ID %d? (y/n): ", id);
    char confirm;
    scanf("%c", &confirm);
    while (getchar() != '\n');
    if (confirm != 'y' && confirm != 'Y') {
        printf("Удаление отменено.\n");
        return;
    }

    for (int i = idx; i < pb->count - 1; i++)
        pb->contacts[i] = pb->contacts[i+1];
    pb->count--;

    if (pb->count == 0) {
        free(pb->contacts);
        pb->contacts = NULL;
    } else {
        Contact *temp = realloc(pb->contacts, pb->count * sizeof(Contact));
        if (temp) pb->contacts = temp;
        else printf("Предупреждение: не удалось уменьшить память.\n");
    }
    printf("Контакт удалён.\n");
}

void search_contacts(PhoneBook *pb) {
    if (pb->count == 0) {
        printf("Телефонная книга пуста.\n");
        return;
    }
    char query[NAME_LEN];
    input_string("Введите строку для поиска (по фамилии или имени): ", query, NAME_LEN, 1);
    int found = 0;
    for (int i = 0; i < pb->count; i++) {
        if (strstr(pb->contacts[i].surname, query) || strstr(pb->contacts[i].name, query)) {
            printf("ID: %d | %s %s %s\n", 
                   pb->contacts[i].id,
                   pb->contacts[i].surname, 
                   pb->contacts[i].name, 
                   pb->contacts[i].patronymic);
            found = 1;
        }
    }
    if (!found) printf("Контакты не найдены.\n");
}
