#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

int main(int argc, char **argv) {
    pid_t pid = fork();

    if (pid < 0) {
        fprintf(stderr, "Fork failed\n"); 
        return 1;
    } else if (pid == 0) {
        
    } else {
    
    }
    
    return 0;
}
