#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Использование: %s <ip_шлюза> <маска> <N>\n", argv[0]);
        return 1;
    }

    struct in_addr addr;
    if (inet_pton(AF_INET, argv[1], &addr) != 1) {
        fprintf(stderr, "Ошибка: неверный IP шлюза\n");
        return 1;
    }
    uint32_t gateway = ntohl(addr.s_addr);

    if (inet_pton(AF_INET, argv[2], &addr) != 1) {
        fprintf(stderr, "Ошибка: неверная маска подсети\n");
        return 1;
    }
    uint32_t mask = ntohl(addr.s_addr);

    long n = atol(argv[3]);
    if (n <= 0) {
        fprintf(stderr, "Ошибка: N должно быть положительным\n");
        return 1;
    }

    srand(time(NULL));

    uint32_t network = gateway & mask;
    unsigned long own = 0, other = 0;

    for (long i = 0; i < n; i++) {
        uint32_t dest = (uint32_t)rand() | ((uint32_t)rand() << 8) |
                        ((uint32_t)rand() << 16) | ((uint32_t)rand() << 24);
        if ((dest & mask) == network)
            own++;
        else
            other++;
    }

    printf("\nСтатистика после обработки %ld пакетов:\n", n);
    printf("  Своя подсеть : %lu пакетов (%.2f%%)\n", own, (double)own / n * 100);
    printf("  Другие сети  : %lu пакетов (%.2f%%)\n", other, (double)other / n * 100);

    return 0;
}