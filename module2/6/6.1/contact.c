#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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

static int compare_contacts(const Contact *a, const Contact *b) {
    int cmp = strcmp(a->surname, b->surname);
    if (cmp != 0) return cmp;
    cmp = strcmp(a->name, b->name);
    if (cmp != 0) return cmp;
    return strcmp(a->patronymic, b->patronymic);
}

static Node* create_node(const Contact *c) {
    Node *node = (Node*)malloc(sizeof(Node));
    if (node) {
        node->data = *c;
        node->prev = node->next = NULL;
    }
    return node;
}

static void insert_sorted(PhoneBook *pb, Node *node) {
    if (pb->head == NULL) {
        pb->head = pb->tail = node;
    } else {
        Node *curr = pb->head;
        while (curr && compare_contacts(&node->data, &curr->data) > 0) {
            curr = curr->next;
        }
        if (curr == NULL) {                     // insert at tail
            node->prev = pb->tail;
            pb->tail->next = node;
            pb->tail = node;
        } else if (curr == pb->head) {          // insert at head
            node->next = pb->head;
            pb->head->prev = node;
            pb->head = node;
        } else {                                // insert before curr
            node->prev = curr->prev;
            node->next = curr;
            curr->prev->next = node;
            curr->prev = node;
        }
    }
    pb->count++;
}

static Node* remove_node(PhoneBook *pb, Node *node) {
    if (node->prev)
        node->prev->next = node->next;
    else
        pb->head = node->next;
    if (node->next)
        node->next->prev = node->prev;
    else
        pb->tail = node->prev;
    node->prev = node->next = NULL;
    pb->count--;
    return node;
}

static Node* find_node_by_id(PhoneBook *pb, int id) {
    Node *curr = pb->head;
    while (curr) {
        if (curr->data.id == id)
            return curr;
        curr = curr->next;
    }
    return NULL;
}

static int get_contact_id(PhoneBook *pb) {
    int id;
    printf("Введите ID контакта: ");
    if (scanf("%d", &id) != 1) {
        while (getchar() != '\n');
        return -1;
    }
    while (getchar() != '\n');
    if (find_node_by_id(pb, id) == NULL)
        return -1;
    return id;
}

void init_phonebook(PhoneBook *pb) {
    pb->head = pb->tail = NULL;
    pb->count = 0;
    pb->next_id = 1;
}

void free_phonebook(PhoneBook *pb) {
    Node *curr = pb->head;
    while (curr) {
        Node *next = curr->next;
        free(curr);
        curr = next;
    }
    pb->head = pb->tail = NULL;
    pb->count = 0;
    pb->next_id = 1;
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

    Node *node = create_node(&new_contact);
    if (!node) {
        printf("Ошибка выделения памяти. Контакт не добавлен.\n");
        pb->next_id--;          // rollback
        return;
    }
    insert_sorted(pb, node);
    printf("Контакт успешно добавлен. ID: %d\n", new_contact.id);
}

void list_contacts(PhoneBook *pb) {
    if (pb->count == 0) {
        printf("Телефонная книга пуста.\n");
        return;
    }
    printf("Список контактов:\n");
    Node *curr = pb->head;
    while (curr) {
        printf("ID: %d | %s %s %s\n",
               curr->data.id,
               curr->data.surname,
               curr->data.name,
               curr->data.patronymic);
        curr = curr->next;
    }
}

void view_contact(PhoneBook *pb, int id) {
    Node *node = find_node_by_id(pb, id);
    if (!node) {
        printf("Контакт с ID %d не найден.\n", id);
        return;
    }
    Contact *c = &node->data;
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
    Node *node = find_node_by_id(pb, id);
    if (!node) return;  // should not happen

    Contact *c = &node->data;
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

    remove_node(pb, node);
    insert_sorted(pb, node);
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
    Node *node = find_node_by_id(pb, id);
    if (!node) return;

    printf("Вы уверены, что хотите удалить контакт с ID %d? (y/n): ", id);
    char confirm;
    scanf("%c", &confirm);
    while (getchar() != '\n');
    if (confirm != 'y' && confirm != 'Y') {
        printf("Удаление отменено.\n");
        return;
    }

    remove_node(pb, node);
    free(node);
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
    Node *curr = pb->head;
    while (curr) {
        if (strstr(curr->data.surname, query) || strstr(curr->data.name, query)) {
            printf("ID: %d | %s %s %s\n",
                   curr->data.id,
                   curr->data.surname,
                   curr->data.name,
                   curr->data.patronymic);
            found = 1;
        }
        curr = curr->next;
    }
    if (!found) printf("Контакты не найдены.\n");
}
