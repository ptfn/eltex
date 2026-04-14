#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>

#define MAX_LINE 1024
#define MAX_ARGS 64

typedef struct {
    char *args[MAX_ARGS];
    char *in_file;
    char *out_file;
} Command;

void print_greeting() {
    printf("Shell v2.0 | ");
}

void parse_command(char *cmd, Command *cmd_struct) {
    char *token;
    int i = 0;
    cmd_struct->in_file = cmd_struct->out_file = NULL;
    
    while ((token = strtok(cmd, " ")) != NULL && i < MAX_ARGS - 1) {
        if (strcmp(token, "<") == 0) {
            cmd_struct->in_file = strtok(NULL, " ");
        } else if (strcmp(token, ">") == 0) {
            cmd_struct->out_file = strtok(NULL, " ");
        } else {
            cmd_struct->args[i++] = token;
        }
        cmd = NULL;
    }
    cmd_struct->args[i] = NULL;
}

void execute(Command *cmds, int num_cmds) {
    int pipes[num_cmds-1][2];
    pid_t pids[num_cmds];
    
    for (int i = 0; i < num_cmds - 1; i++) {
        pipe(pipes[i]);
    }
    
    for (int i = 0; i < num_cmds; i++) {
        pids[i] = fork();
        if (pids[i] == 0) {
            if (cmds[i].in_file) {
                int fd = open(cmds[i].in_file, O_RDONLY);
                dup2(fd, STDIN_FILENO);
                close(fd);
            }
            else if (i == num_cmds - 1 && cmds[i].out_file) {
                int fd = open(cmds[i].out_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }
            
            if (i > 0) {
                dup2(pipes[i-1][0], STDIN_FILENO);
                close(pipes[i-1][0]);
                close(pipes[i-1][1]);
            }
            if (i < num_cmds - 1) {
                dup2(pipes[i][1], STDOUT_FILENO);
                close(pipes[i][0]);
                close(pipes[i][1]);
            }
            
            execvp(cmds[i].args[0], cmds[i].args);
            fprintf(stderr, "Command not found: %s\n", cmds[i].args[0]);
            exit(1);
        }
    }
    
    for (int i = 0; i < num_cmds - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
    for (int i = 0; i < num_cmds; i++) {
        waitpid(pids[i], NULL, 0);
    }
}

int main() {
    char line[MAX_LINE];
    char *cmd_line;
    Command commands[MAX_ARGS];
    int num_commands;
    
    print_greeting();
    
    while (1) {
        printf("> ");
        fflush(stdout);
        
        if (fgets(line, MAX_LINE, stdin) == NULL) break;
        line[strcspn(line, "\n")] = '\0';
        
        if (strcmp(line, "exit") == 0) break;
        
        cmd_line = line;
        num_commands = 0;
        while ((cmd_line = strtok(cmd_line, "|")) != NULL && num_commands < MAX_ARGS) {
            parse_command(cmd_line, &commands[num_commands++]);
            cmd_line = NULL;
        }
        
        if (num_commands > 0) {
            execute(commands, num_commands);
        }
        
        print_greeting();
    }
    
    printf("Goodbye!\n");
    return 0;
}
