#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define PERM_MASK 0777

int parse_numeric_perms(const char *str, mode_t *mode) {
  char *endptr;
  long val = strtol(str, &endptr, 8);
  if (*endptr != '\0' || val < 0 || val > 0777) return -1;
  *mode = (mode_t)val;
  return 0;
}

int parse_symbolic_perms(const char *str, mode_t *mode) {
  if (strlen(str) != 9) return -1;
  mode_t m = 0;

  if (str[0] == 'r')
    m |= S_IRUSR;
  else if (str[0] != '-')
    return -1;
  if (str[1] == 'w')
    m |= S_IWUSR;
  else if (str[1] != '-')
    return -1;
  if (str[2] == 'x')
    m |= S_IXUSR;
  else if (str[2] != '-')
    return -1;

  if (str[3] == 'r')
    m |= S_IRGRP;
  else if (str[3] != '-')
    return -1;
  if (str[4] == 'w')
    m |= S_IWGRP;
  else if (str[4] != '-')
    return -1;
  if (str[5] == 'x')
    m |= S_IXGRP;
  else if (str[5] != '-')
    return -1;

  if (str[6] == 'r')
    m |= S_IROTH;
  else if (str[6] != '-')
    return -1;
  if (str[7] == 'w')
    m |= S_IWOTH;
  else if (str[7] != '-')
    return -1;
  if (str[8] == 'x')
    m |= S_IXOTH;
  else if (str[8] != '-')
    return -1;
  *mode = m;
  return 0;
}

char *format_symbolic_perms(mode_t mode, char *buf, size_t size) {
  if (size < 10) {
    if (size > 0) *buf = '\0';
    return buf;
  }
  mode &= PERM_MASK;
  buf[0] = (mode & S_IRUSR) ? 'r' : '-';
  buf[1] = (mode & S_IWUSR) ? 'w' : '-';
  buf[2] = (mode & S_IXUSR) ? 'x' : '-';
  buf[3] = (mode & S_IRGRP) ? 'r' : '-';
  buf[4] = (mode & S_IWGRP) ? 'w' : '-';
  buf[5] = (mode & S_IXGRP) ? 'x' : '-';
  buf[6] = (mode & S_IROTH) ? 'r' : '-';
  buf[7] = (mode & S_IWOTH) ? 'w' : '-';
  buf[8] = (mode & S_IXOTH) ? 'x' : '-';
  buf[9] = '\0';
  return buf;
}

char *format_numeric_perms(mode_t mode, char *buf, size_t size) {
  snprintf(buf, size, "%03o", (unsigned int)(mode & PERM_MASK));
  return buf;
}

char *format_binary_perms(mode_t mode, char *buf, size_t size) {
  if (size < 10) {
    if (size > 0) *buf = '\0';
    return buf;
  }
  unsigned int m = (unsigned int)(mode & PERM_MASK);
  for (int i = 8; i >= 0; --i) {
    buf[8 - i] = (m & (1 << i)) ? '1' : '0';
  }
  buf[9] = '\0';
  return buf;
}

static mode_t apply_one_mod(mode_t current, const char *mod) {
  if (strlen(mod) < 3) return current;

  char who = mod[0];
  char op = mod[1];
  const char *perms = mod + 2;

  mode_t class_mask = 0;
  if (who == 'u')
    class_mask = S_IRWXU;
  else if (who == 'g')
    class_mask = S_IRWXG;
  else if (who == 'o')
    class_mask = S_IRWXO;
  else if (who == 'a')
    class_mask = S_IRWXU | S_IRWXG | S_IRWXO;
  else
    return current;

  mode_t bit_mask = 0;
  for (const char *p = perms; *p; ++p) {
    switch (*p) {
      case 'r':
        bit_mask |= S_IRUSR;
        break;
      case 'w':
        bit_mask |= S_IWUSR;
        break;
      case 'x':
        bit_mask |= S_IXUSR;
        break;
      default:
        return current;
    }
  }

  if (who == 'a') {
    mode_t expanded = 0;
    if (bit_mask & S_IRUSR) expanded |= S_IRUSR | S_IRGRP | S_IROTH;
    if (bit_mask & S_IWUSR) expanded |= S_IWUSR | S_IWGRP | S_IWOTH;
    if (bit_mask & S_IXUSR) expanded |= S_IXUSR | S_IXGRP | S_IXOTH;
    bit_mask = expanded;
  } else {
    mode_t shifted = 0;
    if (bit_mask & S_IRUSR) shifted |= (class_mask & S_IRWXU);
    if (bit_mask & S_IWUSR) shifted |= (class_mask & S_IRWXU) & S_IWUSR;
    if (bit_mask & S_IXUSR) shifted |= (class_mask & S_IRWXU) & S_IXUSR;

    mode_t new_mask = 0;

    if (who == 'u') {
      new_mask = bit_mask & S_IRWXU;
    } else if (who == 'g') {
      if (bit_mask & S_IRUSR) new_mask |= S_IRGRP;
      if (bit_mask & S_IWUSR) new_mask |= S_IWGRP;
      if (bit_mask & S_IXUSR) new_mask |= S_IXGRP;
    } else if (who == 'o') {
      if (bit_mask & S_IRUSR) new_mask |= S_IROTH;
      if (bit_mask & S_IWUSR) new_mask |= S_IWOTH;
      if (bit_mask & S_IXUSR) new_mask |= S_IXOTH;
    }
    bit_mask = new_mask;
  }

  mode_t result = current;
  if (op == '+') {
    result |= bit_mask;
  } else if (op == '-') {
    result &= ~bit_mask;
  } else if (op == '=') {
    result &= ~class_mask;
    result |= bit_mask;
  } else {
    return current;
  }
  return result;
}

int apply_modification(mode_t current, const char *mod_str, mode_t *new_mode) {
  if (mod_str == NULL || *mod_str == '\0') return -1;

  char *mod_copy = strdup(mod_str);
  if (!mod_copy) return -1;

  mode_t m = current;
  char *token = strtok(mod_copy, ",");
  int ok = 1;

  while (token) {
    mode_t new_m = apply_one_mod(m, token);
    if (new_m == m && strlen(token) < 3) {
      ok = 0;
      break;
    }
    m = new_m;
    token = strtok(NULL, ",");
  }

  free(mod_copy);
  if (!ok) return -1;
  *new_mode = m;

  return 0;
}

int get_file_perms(const char *filename, mode_t *mode) {
  struct stat st;
  if (stat(filename, &st) != 0) return -1;
  *mode = st.st_mode & PERM_MASK;
  return 0;
}

void print_all_representations(mode_t mode) {
  char sym[10], num[10], bin[10];
  format_symbolic_perms(mode, sym, sizeof(sym));
  format_numeric_perms(mode, num, sizeof(num));
  format_binary_perms(mode, bin, sizeof(bin));
  printf("  Символьное:   %s\n", sym);
  printf("  Цифровое:     %s\n", num);
  printf("  Двоичное:     %s\n", bin);
}

int main() {
  setlocale(LC_ALL, "ru_RU.UTF-8");
  mode_t current_mode = 0;
  int have_current = 0;

  while (1) {
    printf("\n=== Меню ===\n");
    printf(
        "1. Ввести права (символьные или цифровые) и показать битовое "
        "представление\n");
    printf("2. Ввести имя файла и показать его права\n");
    printf("3. Изменить права (на основе ранее введённых)\n");
    printf("0. Выход\n");
    printf("Выберите пункт: ");

    int choice;
    if (scanf("%d", &choice) != 1) {
      while (getchar() != '\n')
        ;
      continue;
    }
    getchar();

    if (choice == 0) break;

    if (choice == 1) {
      printf("Введите права (например, 755 или rwxr-xr-x): ");
      char input[20];
      if (!fgets(input, sizeof(input), stdin)) continue;
      input[strcspn(input, "\n")] = '\0';

      mode_t m;
      int ok = 0;

      if (parse_numeric_perms(input, &m) == 0) {
        ok = 1;
      } else if (parse_symbolic_perms(input, &m) == 0) {
        ok = 1;
      } else {
        printf("Ошибка: неверный формат прав.\n");
        continue;
      }
      if (ok) {
        current_mode = m;
        have_current = 1;
        printf("Битовое представление:\n");
        char bin[10];
        format_binary_perms(m, bin, sizeof(bin));
        printf("  %s\n", bin);
      }
    } else if (choice == 2) {
      printf("Введите имя файла: ");
      char filename[256];
      if (!fgets(filename, sizeof(filename), stdin)) continue;
      filename[strcspn(filename, "\n")] = '\0';

      mode_t m;
      if (get_file_perms(filename, &m) != 0) {
        perror("Ошибка stat");
        continue;
      }
      current_mode = m;
      have_current = 1;
      printf("Права доступа файла %s:\n", filename);
      print_all_representations(m);

      printf("\nДля сравнения запустите: ls -l %s\n", filename);
    } else if (choice == 3) {
      if (!have_current) {
        printf("Сначала введите права (пункт 1 или 2).\n");
        continue;
      }
      printf("Текущие права:\n");
      print_all_representations(current_mode);

      printf("Введите команду модификации (например, u+x, g-w, o=rw): ");
      char mod[100];
      if (!fgets(mod, sizeof(mod), stdin)) continue;
      mod[strcspn(mod, "\n")] = '\0';

      mode_t new_mode;
      if (apply_modification(current_mode, mod, &new_mode) != 0) {
        printf("Ошибка: неверная команда модификации.\n");
        continue;
      }
      printf("Новые права:\n");
      print_all_representations(new_mode);
    } else {
      printf("Неверный выбор.\n");
    }
  }
  return 0;
}
