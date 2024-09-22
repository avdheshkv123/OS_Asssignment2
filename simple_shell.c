#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_HISTORY 100

char *history[MAX_HISTORY];
int h_cnt = 0;

void add_to_history(char *command) {
    if(h_cnt < MAX_HISTORY) {
        history[h_cnt] = (char*)malloc(strlen(command)+1);
        if(history[h_cnt]!=NULL){
            strcpy(history[h_cnt],command);
            h_cnt++;
        }  
        else{
            perror("Memory allocation failed");
        }
    }
}

void show_history() {
    for(int i = 0; i < h_cnt; i++) {
        printf("%d: %s", i + 1, history[i]); 
    }
}

void parse_input(char *input, char **args) {
    int i = 0;
    for(args[i] = strtok(input," \n"); args[i]!=NULL; i++){
        args[i+1] = strtok(NULL," \n");
    }
}

void run_command(char **cmd_args){
    pid_t process_id = fork();  //create new process
    int status;
    if(process_id <0){
        perror("Error: failed to create process");
        return;
    }
    
    if(process_id == 0){
        if(execvp(cmd_args[0],cmd_args)<0){
            perror("Error: command execution failed");
            exit(EXIT_FAILURE);
        }
    }
    else{
        if(waitpid(process_id,&status,0)==-1){
            perror("Failed to wait for child process");
            return;
        }
    }

}

void execute_with_pipe(char **first_command, char **second_command) {
    int fd[2];
    pid_t pid1, pid2;

    if (pipe(fd) == -1) {
        perror("Pipe failed");
        return;
    }

    pid1 = fork();

    if (pid1 < 0) {
        perror("Fork failed for first command");
        return;
    }

    else if (pid1 == 0) {
        close(fd[0]); 
        dup2(fd[1], STDOUT_FILENO);
        close(fd[1]);
        if (execvp(first_command[0], first_command) < 0) {
            perror("Execution of first command failed");
        }
        exit(0);
    }

    pid2 = fork();

    if (pid2 < 0) {
        perror("Fork failed for second command");
        return;
    }

    if (pid2 == 0) {
        close(fd[1]);  
        dup2(fd[0], STDIN_FILENO);  
        close(fd[0]); 
        if (execvp(second_command[0], second_command) < 0) {
            perror("Execution of second command failed");
        }
        exit(0);
    }
    wait(NULL);
    wait(NULL);
}

int main() {
    char input[1024];
    char *args[64];
    char *first_command[64], *second_command[64];

    while (1) {
        printf("SimpleShell> ");
        
        if (fgets(input, sizeof(input), stdin) == NULL) {
            continue;  
        }
        add_to_history(input);

        if (strcmp(input, "history\n") == 0) {
            show_history();
            continue;
        }

        char *pipe_position = strchr(input, '|');
        if (pipe_position != NULL) {
            *pipe_position = '\0'; 
            parse_input(input, first_command); 
            parse_input(pipe_position + 1, second_command);
            execute_with_pipe(first_command, second_command); 
        } else {
            parse_input(input, args);
            if (args[0] != NULL) { 
                run_command(args);
            }
        }
    }

    return 0;
}
