#ifndef PHONEBOOK_H
#define PHONEBOOK_H

#define NAME_LEN 50
#define TEXT_LEN 100
#define MULTI_LEN 200

#define BALANCE_THRESHOLD 10

typedef struct {
    int id;
    char surname[NAME_LEN];
    char name[NAME_LEN];
    char patronymic[NAME_LEN];
    char work[TEXT_LEN];
    char position[TEXT_LEN];
    char phones[MULTI_LEN];
    char emails[MULTI_LEN];
    char social[MULTI_LEN];
    char messengers[MULTI_LEN];
} Contact;

typedef struct TreeNode {
    Contact data;
    struct TreeNode *left;
    struct TreeNode *right;
} TreeNode;

typedef struct {
    TreeNode *root;
    int count;          
    int next_id;        
    int mod_count;      
} PhoneBook;

void init_phonebook(PhoneBook *pb);
void free_phonebook(PhoneBook *pb);

void add_contact(PhoneBook *pb);
void list_contacts(PhoneBook *pb);
void view_contact(PhoneBook *pb, int id);
void edit_contact(PhoneBook *pb);
void delete_contact(PhoneBook *pb);
void search_contacts(PhoneBook *pb);
void print_tree(PhoneBook *pb);

#endif
