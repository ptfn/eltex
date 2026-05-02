#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define PERM_MASK 0777

void print_perms(mode_t mode) {
    char sym[10], num[5], bin[10];
    
    // Symbolic
    sym[0] = (mode & S_IRUSR) ? 'r' : '-';
    sym[1] = (mode & S_IWUSR) ? 'w' : '-';
    sym[2] = (mode & S_IXUSR) ? 'x' : '-';
    sym[3] = (mode & S_IRGRP) ? 'r' : '-';
    sym[4] = (mode & S_IWGRP) ? 'w' : '-';
    sym[5] = (mode & S_IXGRP) ? 'x' : '-';
    sym[6] = (mode & S_IROTH) ? 'r' : '-';
    sym[7] = (mode & S_IWOTH) ? 'w' : '-';
    sym[8] = (mode & S_IXOTH) ? 'x' : '-';
    sym[9] = '\0';
    
    // Numeric
    snprintf(num, sizeof(num), "%03o", mode & PERM_MASK);
    
    // Binary
    unsigned int m = mode & PERM_MASK;
    for (int i = 8; i >= 0; i--)
        bin[8 - i] = (m & (1 << i)) ? '1' : '0';
    bin[9] = '\0';
    
    printf("  Символьное: %s\n", sym);
    printf("  Цифровое:   %s\n", num);
    printf("  Битовое:    %s\n", bin);
}

int parse_perms(const char *s, mode_t *mode) {
    if (strlen(s) == 9) {  // Symbolic (rwxr-xr-x)
        *mode = 0;
        for (int i = 0; i < 9; i++) {
            if (s[i] != 'r' && s[i] != 'w' && s[i] != 'x' && s[i] != '-')
                return -1;
            switch (i) {
                case 0: if (s[i]=='r') *mode|=S_IRUSR; break;
                case 1: if (s[i]=='w') *mode|=S_IWUSR; break;
                case 2: if (s[i]=='x') *mode|=S_IXUSR; break;
                case 3: if (s[i]=='r') *mode|=S_IRGRP; break;
                case 4: if (s[i]=='w') *mode|=S_IWGRP; break;
                case 5: if (s[i]=='x') *mode|=S_IXGRP; break;
                case 6: if (s[i]=='r') *mode|=S_IROTH; break;
                case 7: if (s[i]=='w') *mode|=S_IWOTH; break;
                case 8: if (s[i]=='x') *mode|=S_IXOTH; break;
            }
        }
        return 0;
    } else if (strlen(s) <= 4) {  // Numeric (755)
        char *end;
        long v = strtol(s, &end, 8);
        if (*end || v < 0 || v > 0777) return -1;
        *mode = v;
        return 0;
    }
    return -1;
}

int modify_perms(mode_t cur, const char *mod_str, mode_t *new_mode) {
    char copy[64];
    strncpy(copy, mod_str, sizeof(copy) - 1);
    copy[sizeof(copy) - 1] = '\0';
    
    char *tok = strtok(copy, ",");
    mode_t result = cur;
    
    while (tok) {
        if (strlen(tok) < 3) return -1;
        
        char who = tok[0], op = tok[1];
        const char *p = tok + 2;
        
        mode_t bits = 0;
        for (; *p; p++) {
            if (*p == 'r') bits |= S_IRUSR;
            else if (*p == 'w') bits |= S_IWUSR;
            else if (*p == 'x') bits |= S_IXUSR;
            else return -1;
        }
        
        // Expand to all classes for 'a'
        if (who == 'a') {
            if (bits & S_IRUSR) bits = (bits & ~S_IRUSR) | S_IRUSR|S_IRGRP|S_IROTH;
            if (bits & S_IWUSR) bits = (bits & ~S_IWUSR) | S_IWUSR|S_IWGRP|S_IWOTH;
            if (bits & S_IXUSR) bits = (bits & ~S_IXUSR) | S_IXUSR|S_IXGRP|S_IXOTH;
        }
        
        // Map u/g/o to actual bits
        mode_t mapped = 0;
        if (who == 'u') {
            if (bits & S_IRUSR) mapped |= S_IRUSR;
            if (bits & S_IWUSR) mapped |= S_IWUSR;
            if (bits & S_IXUSR) mapped |= S_IXUSR;
        } else if (who == 'g') {
            if (bits & S_IRUSR) mapped |= S_IRGRP;
            if (bits & S_IWUSR) mapped |= S_IWGRP;
            if (bits & S_IXUSR) mapped |= S_IXGRP;
        } else if (who == 'o') {
            if (bits & S_IRUSR) mapped |= S_IROTH;
            if (bits & S_IWUSR) mapped |= S_IWOTH;
            if (bits & S_IXUSR) mapped |= S_IXOTH;
        } else {
            return -1;
        }
        
        if (op == '+') result |= mapped;
        else if (op == '-') result &= ~mapped;
        else if (op == '=') { result &= ~(who=='u'?S_IRWXU:who=='g'?S_IRWXG:S_IRWXO); result |= mapped; }
        else return -1;
        
        tok = strtok(NULL, ",");
    }
    
    *new_mode = result;
    return 0;
}

int main() {
    setlocale(LC_ALL, "ru_RU.UTF-8");
    mode_t cur = 0;
    int active = 0;
    
    while (1) {
        printf("\n=== Меню ===\n");
        printf("1. Ввести права → показать биты\n");
        printf("2. Ввести файл → показать все представления\n");
        printf("3. Изменить права\n");
        printf("0. Выход\n");
        printf("Выбор: ");
        
        int ch;
        if (scanf("%d", &ch) != 1) { getchar(); continue; }
        getchar();
        
        if (ch == 0) break;
        
        if (ch == 1) {
            printf("Права (755 или rwxr-xr-x): ");
            char s[32];
            if (!fgets(s, sizeof(s), stdin)) continue;
            s[strcspn(s, "\n")] = '\0';
            
            if (parse_perms(s, &cur) == 0) {
                active = 1;
                char bin[10];
                unsigned int m = cur & PERM_MASK;
                for (int i = 8; i >= 0; i--) bin[8-i] = (m & (1<<i)) ? '1' : '0';
                bin[9] = '\0';
                printf("Биты: %s\n", bin);
            } else {
                printf("Ошибка: неверный формат\n");
            }
        } else if (ch == 2) {
            printf("Имя файла: ");
            char name[256];
            if (!fgets(name, sizeof(name), stdin)) continue;
            name[strcspn(name, "\n")] = '\0';
            
            struct stat st;
            if (stat(name, &st) == 0) {
                active = 1;
                cur = st.st_mode & PERM_MASK;
                printf("Права файла %s:\n", name);
                print_perms(cur);
                printf("\nПроверка: ls -l %s\n", name);
            } else {
                perror("Ошибка");
            }
        } else if (ch == 3) {
            if (!active) { printf("Сначала введите права (пункт 1 или 2)\n"); continue; }
            printf("Текущие:\n");
            print_perms(cur);
            
            printf("Модификация (u+x, g-w, o=rw): ");
            char mod[64];
            if (!fgets(mod, sizeof(mod), stdin)) continue;
            mod[strcspn(mod, "\n")] = '\0';
            
            mode_t new_mode;
            if (modify_perms(cur, mod, &new_mode) == 0) {
                printf("Новые права:\n");
                print_perms(new_mode);
            } else {
                printf("Ошибка: неверный формат\n");
            }
        }
    }
    return 0;
}