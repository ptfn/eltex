#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char *argv[]) {
  if (argc != 4) {
    fprintf(stderr, "Usage: %s <gateway_ip> <subnet_mask> <N>\n", argv[0]);
    return EXIT_FAILURE;
  }

  char *endptr;
  long n = strtol(argv[3], &endptr, 10);
  if (*endptr || n <= 0) {
    fprintf(stderr, "Error: N must be a positive integer.\n");
    return EXIT_FAILURE;
  }

  struct in_addr addr;
  if (inet_pton(AF_INET, argv[1], &addr) != 1) {
    fprintf(stderr, "Error: Invalid gateway IP.\n");
    return EXIT_FAILURE;
  }
  uint32_t gw_ip = ntohl(addr.s_addr);

  if (inet_pton(AF_INET, argv[2], &addr) != 1) {
    fprintf(stderr, "Error: Invalid subnet mask.\n");
    return EXIT_FAILURE;
  }
  uint32_t mask = ntohl(addr.s_addr);

  srand(time(NULL));

  unsigned long own = 0;
  for (long i = 0; i < n; i++) {
    uint32_t dest = (uint32_t)rand() | ((uint32_t)rand() << 8) |
                    ((uint32_t)rand() << 16) | ((uint32_t)rand() << 24);
    if ((dest & mask) == (gw_ip & mask))
      own++;
  }

  printf("Statistics after processing %ld packets:\n", n);
  printf("  Own subnet : %lu packets (%.2f%%)\n", own, (double)own / n * 100);
  printf("  Other nets : %lu packets (%.2f%%)\n", n - own,
         (double)(n - own) / n * 100);

  return EXIT_SUCCESS;
}
