/*
Author: Assem Samy
File: shell_os.c
Comment: Contains all function definitions and main function used in shell
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>

#define MAXSIZE 1024
#define MAX_ARGUMENTS 64

FILE *openfil;

// Signal handler for SIGCHLD
void childSignalHandler(int sig) {
    fprintf(openfil, "Child process was terminated\n");
    fflush(openfil);
    while (waitpid(-1, NULL, WNOHANG) > 0);
}


void externalCommandExecution(char **args, int backgroundHandler) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed to make child");
        return;
    }
    if (pid == 0) { 
        if (execvp(args[0], args) == -1) {
            perror("execvp failed:");
            exit(EXIT_FAILURE);
        }
    } else { 
        if (!backgroundHandler) {
            int status;
            waitpid(pid, &status, 0);
            if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
                fprintf(stderr, "Command failed with exit code %d\n", WEXITSTATUS(status));
            }
        } else {
            printf("backgroundHandler process with pid %d\n", pid);
        }
    }
}


int builtinCommandsHandling(char **args) {
    if (strcmp(args[0], "exit") == 0) {
        exit(EXIT_SUCCESS);
    } else if (strcmp(args[0], "cd") == 0) {
        if (args[1] == NULL || strcmp(args[1], "~") == 0) {
            chdir(getenv("HOME"));
        } else if (strcmp(args[1], "..") == 0) {
            chdir("..");
        } else {
            if (chdir(args[1]) != 0) {
                perror("cd");
            }
        }
        return 1;
    } else if (strcmp(args[0], "export") == 0) {
        if (args[1] != NULL) {
            char *equal_sign = strchr(args[1], '=');
            if (equal_sign != NULL) {
                *equal_sign = '\0';
                char *name = args[1];
                char *value = equal_sign + 1;
                if (value[0] == '\"' && value[strlen(value)-1] == '\"') {
                    value[strlen(value)-1] = '\0';
                    value++;
                }
                setenv(name, value, 1);
            }
        }
        return 1;
    } else if (strcmp(args[0], "echo") == 0) {
        for (int i = 1; args[i] != NULL; i++) {
            printf("%s ", args[i]);
        }
        printf("\n");
        return 1;
    }
    return 0;
}

void substitute_variables(char **args) {
    char *completeArgs[MAX_ARGUMENTS];
    int new_arg_index = 0;
    
    for (int i = 0; args[i] != NULL; i++) {
        if (args[i][0] == '$') {
            if (strchr(args[i], ' ') == NULL) {
                char *var_name = args[i] + 1;
                char *var_value = getenv(var_name);
                if (var_value != NULL) {
                    if (strchr(var_value, ' ') != NULL) {
                        char *dup = strdup(var_value);
                        if (!dup) {
                            perror("strdup failed");
                            exit(EXIT_FAILURE);
                        }
                        char *token;
                        char *saveptr;
                        token = strtok_r(dup, " ", &saveptr);
                        while (token != NULL) {
                            completeArgs[new_arg_index++] = strdup(token);
                            token = strtok_r(NULL, " ", &saveptr);
                        }
                        free(dup);
                        free(args[i]);
                        continue;
                    } else {
                        free(args[i]);
                        completeArgs[new_arg_index++] = strdup(var_value);
                        continue;
                    }
                } else {
                    completeArgs[new_arg_index++] = args[i];
                    continue;
                }
            } else {
                char out[MAXSIZE];
                out[0] = '\0';
                char *p = args[i];
                while (*p) {
                    if (*p == '$') {
                        p++;
                        char varname[128];
                        int j = 0;
                        while (*p && !isspace(*p)) {
                            varname[j++] = *p;
                            p++;
                        }
                        varname[j] = '\0';
                        char *valOfVar = getenv(varname);
                        if (valOfVar) {
                            strcat(out, valOfVar);
                        }
                    } else {
                        int l = strlen(out);
                        out[l] = *p;
                        out[l+1] = '\0';
                        p++;
                    }
                }
                free(args[i]);
                completeArgs[new_arg_index++] = strdup(out);
                continue;
            }
        } else {
            completeArgs[new_arg_index++] = args[i];
        }
    }
    completeArgs[new_arg_index] = NULL;
    for (int i = 0; i < new_arg_index; i++) {
        args[i] = completeArgs[i];
    }
    args[new_arg_index] = NULL;
}

void inputTokenizing(char *input, char **args, int *bkhand) {
    int i = 0;
    *bkhand = 0;
    
    if (strncmp(input, "export", 6) == 0 && isspace(input[6])) {
        args[i++] = strdup("export");
        char *ptrinc = input + 6;
        while (*ptrinc && isspace(*ptrinc)) {
            ptrinc++;
        }
        char *newLineAtEnd = strchr(ptrinc, '\n');
        if (newLineAtEnd) {
            *newLineAtEnd = '\0';
        }
        args[i++] = strdup(ptrinc);
        args[i] = NULL;
    
        char *key = strtok(ptrinc, "=");
        char *value = strtok(NULL, "=");
        if (key && value) {
            setenv(key, value, 1);  
        } else {
            fprintf(stderr, "export: invalid format, use export VAR=VALUE\n");
        }
    
        return;
    }
    
    
    //for rest of commands
    char *ptrinc = input;
    while (*ptrinc) {
        while (*ptrinc && isspace(*ptrinc))
            ptrinc++;
        if (!*ptrinc)
            break;
        char token[MAXSIZE];
        int j = 0;
        if (*ptrinc == '\"') {
            ptrinc++; 
            while (*ptrinc && *ptrinc != '\"') {
                token[j++] = *ptrinc;
                ptrinc++;
            }
            if (*ptrinc == '\"')
                ptrinc++; 
        } else {
            while (*ptrinc && !isspace(*ptrinc)) {
                token[j++] = *ptrinc;
                ptrinc++;
            }
        }
        token[j] = '\0';
        if (strcmp(token, "&") == 0) {
            *bkhand = 1;
        } else {
            args[i++] = strdup(token);
        }
    }
    args[i] = NULL;
    if (i > 0 && strcmp(args[0], "export") != 0) {
        substitute_variables(args);
    }
}

int main() {
    char input[MAXSIZE];
    char *args[MAX_ARGUMENTS];
    int backgroundHandler;
    
    openfil = fopen("assem.log", "a");
    if (openfil == NULL) {
        perror("fopen failed to open file shell.log");
        exit(EXIT_FAILURE);
    }
    
    signal(SIGCHLD, childSignalHandler);
    
    while (1) {
        printf("myshell> ");
        fflush(stdout);
        
        if (fgets(input, MAXSIZE, stdin) == NULL){
            break;
        }
        size_t len = strlen(input);
        if (len > 0 && input[len - 1] == '\n'){
            input[len - 1] = '\0';
        }
        
        inputTokenizing(input, args, &backgroundHandler);
        
        if (args[0] == NULL){
            continue;
        }
        if (builtinCommandsHandling(args) == 1){
            continue;
        }
        externalCommandExecution(args, backgroundHandler);
        
        for (int i = 0; args[i] != NULL; i++) {
            free(args[i]);
        }
    }
    
    fclose(openfil);
    return 0;
}
