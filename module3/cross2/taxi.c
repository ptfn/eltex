#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <poll.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>

#define MAX_DRIVERS 64
#define BUFFER_SIZE 256
#define SOCKET_PATH "/tmp/taxi_cli.sock"

typedef struct {
    int pid;
    int used;
    int busy;
    int remaining;
} driver_t;

driver_t drivers[MAX_DRIVERS];
int driver_count = 0;

int find_driver_idx(int pid) {
    for (int i = 0; i < MAX_DRIVERS; i++) {
        if (drivers[i].used && drivers[i].pid == pid)
            return i;
    }
    return -1;
}

// Таймер для driver
static int timer_expired = 0;

void sigalrm_handler(int sig) {
    timer_expired = 1;
}

// ==================== DRIVER ====================
void run_driver(int my_pid) {
    char sfd_str[32];
    snprintf(sfd_str, sizeof(sfd_str), "%d", my_pid);
    
    char socket_path[64];
    snprintf(socket_path, sizeof(socket_path), "/tmp/taxi_driver_%d.sock", my_pid);
    
    unlink(socket_path);
    
    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(1);
    }
    
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, socket_path);
    
    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(sockfd);
        exit(1);
    }
    
    listen(sockfd, 5);
    
    // Настраиваем таймер
    struct sigaction sa;
    sa.sa_handler = sigalrm_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGALRM, &sa, NULL);
    
    int busy = 0;
    int task_duration = 0;
    int start_time = 0;
    
    printf("Driver %d ready on %s\n", my_pid, socket_path);
    fflush(stdout);
    
    while (1) {
        struct pollfd pfd;
        pfd.fd = sockfd;
        pfd.events = POLLIN;
        
        int timeout_ms;
        if (busy) {
            int elapsed = time(NULL) - start_time;
            int remaining = task_duration - elapsed;
            if (remaining <= 0) {
                busy = 0;
                printf("Driver %d: now Available\n", my_pid);
                fflush(stdout);
                timeout_ms = -1;
            } else {
                timeout_ms = remaining * 1000;
            }
        } else {
            timeout_ms = -1;
        }
        
        int ret = poll(&pfd, 1, timeout_ms);
        
        if (busy) {
            int elapsed = time(NULL) - start_time;
            if (elapsed >= task_duration) {
                busy = 0;
                printf("Driver %d: now Available\n", my_pid);
                fflush(stdout);
            }
        }
        
        if (ret < 0) continue;
        if (ret == 0) continue;
        
        if (pfd.revents & POLLIN) {
            int client_fd = accept(sockfd, NULL, NULL);
            if (client_fd < 0) continue;
            
            char cmd[BUFFER_SIZE];
            memset(cmd, 0, BUFFER_SIZE);
            read(client_fd, cmd, BUFFER_SIZE - 1);
            
            char resp[BUFFER_SIZE];
            
            if (strncmp(cmd, "STATUS", 6) == 0) {
                if (busy) {
                    int elapsed = time(NULL) - start_time;
                    int remaining = task_duration - elapsed;
                    if (remaining < 0) remaining = 0;
                    snprintf(resp, sizeof(resp), "Busy %d\n", remaining);
                } else {
                    snprintf(resp, sizeof(resp), "Available\n");
                }
            }
            else if (strncmp(cmd, "TASK ", 5) == 0) {
                int duration = atoi(cmd + 5);
                if (duration <= 0) {
                    snprintf(resp, sizeof(resp), "Error: invalid duration\n");
                } else if (busy) {
                    int elapsed = time(NULL) - start_time;
                    int remaining = task_duration - elapsed;
                    if (remaining < 0) remaining = 0;
                    snprintf(resp, sizeof(resp), "Busy %d\n", remaining);
                } else {
                    busy = 1;
                    task_duration = duration;
                    start_time = time(NULL);
                    snprintf(resp, sizeof(resp), "OK %d\n", duration);
                    printf("Driver %d: Busy for %d sec\n", my_pid, duration);
                    fflush(stdout);
                }
            }
            else if (strncmp(cmd, "QUIT", 4) == 0) {
                snprintf(resp, sizeof(resp), "OK\n");
                write(client_fd, resp, strlen(resp));
                close(client_fd);
                break;
            }
            
            write(client_fd, resp, strlen(resp));
            close(client_fd);
        }
    }
    
    close(sockfd);
    unlink(socket_path);
    exit(0);
}

// ==================== CLI ====================

void cmd_create_driver() {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return;
    }
    if (pid == 0) {
        run_driver(getpid());
    } else {
        usleep(100000);  // Даём время создать сокет
        for (int i = 0; i < MAX_DRIVERS; i++) {
            if (!drivers[i].used) {
                drivers[i].used = 1;
                drivers[i].pid = pid;
                drivers[i].busy = 0;
                drivers[i].remaining = 0;
                driver_count++;
                printf("Driver created: PID %d\n", pid);
                return;
            }
        }
    }
}

int connect_driver(int pid) {
    char socket_path[64];
    snprintf(socket_path, sizeof(socket_path), "/tmp/taxi_driver_%d.sock", pid);
    
    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0) return -1;
    
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, socket_path);
    
    if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(sockfd);
        return -1;
    }
    return sockfd;
}

void cmd_send_task(int pid, int timer) {
    int idx = find_driver_idx(pid);
    if (idx < 0) {
        printf("Error: driver %d not found\n", pid);
        return;
    }
    
    int sockfd = connect_driver(pid);
    if (sockfd < 0) {
        printf("Error: cannot connect to driver %d\n", pid);
        drivers[idx].used = 0;
        return;
    }
    
    char cmd[BUFFER_SIZE];
    snprintf(cmd, sizeof(cmd), "TASK %d", timer);
    write(sockfd, cmd, strlen(cmd));
    
    char resp[BUFFER_SIZE];
    read(sockfd, resp, BUFFER_SIZE - 1);
    printf("%s", resp);
    
    if (strncmp(resp, "OK", 2) == 0) {
        drivers[idx].busy = 1;
        drivers[idx].remaining = timer;
    }
    
    close(sockfd);
}

void cmd_get_status(int pid) {
    int idx = find_driver_idx(pid);
    if (idx < 0) {
        printf("Error: driver %d not found\n", pid);
        return;
    }
    
    int sockfd = connect_driver(pid);
    if (sockfd < 0) {
        printf("Error: cannot connect to driver %d (process died?)\n", pid);
        drivers[idx].used = 0;
        return;
    }
    
    write(sockfd, "STATUS", 6);
    
    char resp[BUFFER_SIZE];
    read(sockfd, resp, BUFFER_SIZE - 1);
    printf("Driver %d: %s", pid, resp);
    
    close(sockfd);
}

void cmd_get_drivers() {
    printf("Active drivers:\n");
    for (int i = 0; i < MAX_DRIVERS; i++) {
        if (drivers[i].used) {
            int sockfd = connect_driver(drivers[i].pid);
            if (sockfd >= 0) {
                write(sockfd, "STATUS", 6);
                char resp[BUFFER_SIZE];
                read(sockfd, resp, BUFFER_SIZE - 1);
                printf("  PID %d: %s", drivers[i].pid, resp);
                close(sockfd);
            } else {
                printf("  PID %d: disconnected (dead?)\n", drivers[i].pid);
                drivers[i].used = 0;
            }
        }
    }
}

void cleanup() {
    for (int i = 0; i < MAX_DRIVERS; i++) {
        if (drivers[i].used) {
            int sockfd = connect_driver(drivers[i].pid);
            if (sockfd >= 0) {
                write(sockfd, "QUIT", 4);
                close(sockfd);
            }
            kill(drivers[i].pid, SIGTERM);
            waitpid(drivers[i].pid, NULL, WNOHANG);
            
            char socket_path[64];
            snprintf(socket_path, sizeof(socket_path), "/tmp/taxi_driver_%d.sock", drivers[i].pid);
            unlink(socket_path);
        }
    }
}

int main() {
    char input[BUFFER_SIZE];
    
    printf("=== Taxi CLI ===\n");
    printf("Commands: create_driver, send_task <pid> <timer>, get_status <pid>, get_drivers, exit\n\n");
    
    while (1) {
        printf("> ");
        fflush(stdout);
        
        if (fgets(input, BUFFER_SIZE, stdin) == NULL) break;
        input[strcspn(input, "\n")] = '\0';
        
        if (strcmp(input, "exit") == 0) {
            break;
        }
        else if (strcmp(input, "create_driver") == 0) {
            cmd_create_driver();
        }
        else if (strncmp(input, "send_task ", 10) == 0) {
            int pid, timer;
            if (sscanf(input + 10, "%d %d", &pid, &timer) == 2) {
                cmd_send_task(pid, timer);
            } else {
                printf("Usage: send_task <pid> <timer>\n");
            }
        }
        else if (strncmp(input, "get_status ", 11) == 0) {
            int pid = atoi(input + 11);
            if (pid > 0) cmd_get_status(pid);
            else printf("Usage: get_status <pid>\n");
        }
        else if (strcmp(input, "get_drivers") == 0) {
            cmd_get_drivers();
        }
        else {
            printf("Unknown command\n");
        }
    }
    
    cleanup();
    printf("CLI exit\n");
    return 0;
}