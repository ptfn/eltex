#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "contact.h"

static void clear_input_buffer(void) {
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF);
}

static void input_string(const char *prompt, char *buffer, int size, int required) {
    while (1) {
        printf("%s", prompt);
        if (fgets(buffer, size, stdin) != NULL) {
            size_t len = strlen(buffer);
            if (len > 0 && buffer[len-1] == '\n')
                buffer[len-1] = '\0';
            else
                clear_input_buffer();
            if (required && strlen(buffer) == 0) {
                printf("Это поле обязательно. Пожалуйста, введите значение.\n");
                continue;
            }
            break;
        }
    }
}

static int read_contact_id(PhoneBook *pb) {
    int id;
    list_contacts(pb);
    printf("Введите ID контакта: ");
    if (scanf("%d", &id) != 1) {
        clear_input_buffer();
        printf("Неверный ID.\n");
        return -1;
    }
    clear_input_buffer();
    return id;
}

static void edit_field(const char *label, char *current, char *buffer, int size) {
    printf("Текущее %s: %s\n", label, current);
    printf("Новое %s: ", label);
    input_string("", buffer, size, 0);
    if (strlen(buffer) > 0) strcpy(current, buffer);
}

// Создание нового узла дерева
static TreeNode* create_node(Contact c) {
    TreeNode *node = (TreeNode*)malloc(sizeof(TreeNode));
    if (!node) return NULL;
    node->data = c;
    node->left = node->right = NULL;
    return node;
}

// Рекурсивное освобождение дерева
static void free_tree(TreeNode *node) {
    if (node) {
        free_tree(node->left);
        free_tree(node->right);
        free(node);
    }
}

// Вставка узла по ID (ключ – id)
static TreeNode* insert_node(TreeNode *node, Contact c, int *count) {
    if (!node) {
        (*count)++;
        return create_node(c);
    }
    if (c.id < node->data.id)
        node->left = insert_node(node->left, c, count);
    else if (c.id > node->data.id)
        node->right = insert_node(node->right, c, count);
    // Если ID совпадает – не вставляем (такого быть не должно)
    return node;
}

// Поиск узла по ID
static TreeNode* find_node(TreeNode *node, int id) {
    if (!node || node->data.id == id)
        return node;
    if (id < node->data.id)
        return find_node(node->left, id);
    else
        return find_node(node->right, id);
}

// Удаление узла по ID
static TreeNode* delete_node(TreeNode *node, int id, int *count) {
    if (!node) return NULL;
    if (id < node->data.id)
        node->left = delete_node(node->left, id, count);
    else if (id > node->data.id)
        node->right = delete_node(node->right, id, count);
    else {
        // Нашли узел для удаления
        if (!node->left) {
            TreeNode *right = node->right;
            free(node);
            (*count)--;
            return right;
        } else if (!node->right) {
            TreeNode *left = node->left;
            free(node);
            (*count)--;
            return left;
        } else {
            // Ищем минимальный элемент в правом поддереве
            TreeNode *min = node->right;
            while (min->left)
                min = min->left;
            node->data = min->data;  // копируем данные
            node->right = delete_node(node->right, min->data.id, count);
        }
    }
    return node;
}

// Визуализация дерева (повёрнутый на 90° вид)
static void print_tree_visual(TreeNode *node, int level) {
    if (!node) return;
    print_tree_visual(node->right, level + 1);
    for (int i = 0; i < level; i++) printf("        ");
    printf("├── ID:%d %s %s\n", node->data.id, node->data.surname, node->data.name);
    print_tree_visual(node->left, level + 1);
}

void print_tree(PhoneBook *pb) {
    if (pb->count == 0) {
        printf("Дерево пусто.\n");
        return;
    }
    printf("Структура дерева (правое поддерево сверху):\n");
    print_tree_visual(pb->root, 0);
}

// Сбор узлов в массив (inorder обход)
static void inorder_collect(TreeNode *node, Contact **arr, int *idx) {
    if (!node) return;
    inorder_collect(node->left, arr, idx);
    arr[(*idx)++] = &node->data;
    inorder_collect(node->right, arr, idx);
}

// Построение сбалансированного дерева из отсортированного массива указателей на контакты
static TreeNode* build_balanced(Contact **arr, int start, int end) {
    if (start > end) return NULL;
    int mid = (start + end) / 2;
    TreeNode *node = create_node(*arr[mid]);
    node->left = build_balanced(arr, start, mid - 1);
    node->right = build_balanced(arr, mid + 1, end);
    return node;
}

// Полная перебалансировка дерева
static void balance_tree(PhoneBook *pb) {
    if (pb->count == 0) {
        pb->root = NULL;
        return;
    }
    // Собираем все узлы в массив
    Contact **arr = (Contact**)malloc(pb->count * sizeof(Contact*));
    if (!arr) {
        printf("Ошибка памяти при балансировке.\n");
        return;
    }
    int idx = 0;
    inorder_collect(pb->root, arr, &idx);
    // Освобождаем старое дерево
    free_tree(pb->root);
    // Строим новое сбалансированное дерево
    pb->root = build_balanced(arr, 0, pb->count - 1);
    free(arr);
    pb->mod_count = 0;  // сбрасываем счётчик
}

// Инициализация телефонной книги
void init_phonebook(PhoneBook *pb) {
    pb->root = NULL;
    pb->count = 0;
    pb->next_id = 1;
    pb->mod_count = 0;
}

// Освобождение памяти
void free_phonebook(PhoneBook *pb) {
    free_tree(pb->root);
    pb->root = NULL;
    pb->count = 0;
    pb->next_id = 1;
    pb->mod_count = 0;
}

// Добавление контакта
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

    pb->root = insert_node(pb->root, new_contact, &pb->count);
    if (!pb->root) {
        printf("Ошибка выделения памяти. Контакт не добавлен.\n");
        pb->next_id--;
        return;
    }
    pb->mod_count++;
    if (pb->mod_count >= BALANCE_THRESHOLD)
        balance_tree(pb);
    printf("Контакт успешно добавлен. ID: %d\n", new_contact.id);
}

// Вывод всех контактов (inorder обход)
static void inorder_print(TreeNode *node) {
    if (!node) return;
    inorder_print(node->left);
    printf("ID: %d | %s %s %s\n",
           node->data.id,
           node->data.surname,
           node->data.name,
           node->data.patronymic);
    inorder_print(node->right);
}

void list_contacts(PhoneBook *pb) {
    if (pb->count == 0) {
        printf("Телефонная книга пуста.\n");
        return;
    }
    printf("Список контактов:\n");
    inorder_print(pb->root);
}

// Просмотр контакта по ID
void view_contact(PhoneBook *pb, int id) {
    TreeNode *node = find_node(pb->root, id);
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

// Редактирование контакта
void edit_contact(PhoneBook *pb) {
    if (pb->count == 0) {
        printf("Нет контактов для редактирования.\n");
        return;
    }
    int id = read_contact_id(pb);
    if (id < 0) return;

    TreeNode *node = find_node(pb->root, id);
    if (!node) {
        printf("Контакт с ID %d не найден.\n", id);
        return;
    }
    Contact *c = &node->data;
    printf("Редактирование контакта ID %d. Оставьте поле пустым, чтобы сохранить текущее значение.\n", id);

    char buffer[MULTI_LEN];
    edit_field("фамилия", c->surname, buffer, NAME_LEN);
    edit_field("имя", c->name, buffer, NAME_LEN);
    edit_field("отчество", c->patronymic, buffer, NAME_LEN);
    edit_field("место работы", c->work, buffer, TEXT_LEN);
    edit_field("должность", c->position, buffer, TEXT_LEN);
    edit_field("телефоны", c->phones, buffer, MULTI_LEN);
    edit_field("email", c->emails, buffer, MULTI_LEN);
    edit_field("соцсети", c->social, buffer, MULTI_LEN);
    edit_field("мессенджеры", c->messengers, buffer, MULTI_LEN);

    printf("Контакт обновлён.\n");
}

// Удаление контакта
void delete_contact(PhoneBook *pb) {
    if (pb->count == 0) {
        printf("Нет контактов для удаления.\n");
        return;
    }
    list_contacts(pb);
    int id;
    printf("Введите ID контакта: ");
    if (scanf("%d", &id) != 1) {
        while (getchar() != '\n');
        printf("Неверный ID.\n");
        return;
    }
    while (getchar() != '\n');

    if (!find_node(pb->root, id)) {
        printf("Контакт с ID %d не найден.\n", id);
        return;
    }
    printf("Вы уверены, что хотите удалить контакт с ID %d? (y/n): ", id);
    char confirm;
    scanf("%c", &confirm);
    while (getchar() != '\n');
    if (confirm != 'y' && confirm != 'Y') {
        printf("Удаление отменено.\n");
        return;
    }

    pb->root = delete_node(pb->root, id, &pb->count);
    pb->mod_count++;
    if (pb->mod_count >= BALANCE_THRESHOLD)
        balance_tree(pb);
    printf("Контакт удалён.\n");
}

// Поиск по фамилии или имени (обход всего дерева)
static void search_inorder(TreeNode *node, const char *query, int *found) {
    if (!node) return;
    search_inorder(node->left, query, found);
    if (strstr(node->data.surname, query) || strstr(node->data.name, query)) {
        printf("ID: %d | %s %s %s\n",
               node->data.id,
               node->data.surname,
               node->data.name,
               node->data.patronymic);
        (*found) = 1;
    }
    search_inorder(node->right, query, found);
}

void search_contacts(PhoneBook *pb) {
    if (pb->count == 0) {
        printf("Телефонная книга пуста.\n");
        return;
    }
    char query[NAME_LEN];
    input_string("Введите строку для поиска (по фамилии или имени): ", query, NAME_LEN, 1);
    int found = 0;
    search_inorder(pb->root, query, &found);
    if (!found) printf("Контакты не найдены.\n");
}
