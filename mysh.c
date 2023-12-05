#include <stdio.h> 
#include <stdlib.h> 
#include <stdbool.h>
#include <unistd.h>
#include <string.h> 

#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/wait.h>
#include <glob.h>

#define MAX_COMMAND_LENGTH 256 //assume no commands get cut off
#define MAX_ARGS 12

//malloc()'s a new string that contains the full path name of token/program
//just return original token if the program is a built-in OR already a pathname
char* name_builder(char* token){
    //pathname OR built-in: cd, pwd, which
    if(token == NULL || strchr(token, '/')!= NULL || strcmp(token, "cd") ==0 || strcmp(token, "pwd") ==0 || strcmp(token, "which") ==0){
        return token;
    }
    //bare name. search for it
    //char path[14+strlen(token)];
    char* new_token = malloc(strlen(token)+1+1);
    new_token[0] = '/';
    strcpy(new_token+1, token);

    char* path = malloc(14+strlen(new_token)+1);
    strcpy(path,"/usr/local/bin");
    strcat(path, new_token);
    if(access(path, F_OK)==0){
        free(new_token);
        return path;
    }
    memset(path,0,strlen(path));
    strcpy(path,"/usr/bin");
    strcat(path, new_token);
    if(access(path, F_OK)==0){
        free(new_token);
        return path;
    }
    memset(path,0,strlen(path));
    strcpy(path,"/bin");
    strcat(path, new_token);
    if(access(path, F_OK)==0){
        free(new_token);
        return path;
    }
    free(path);
    free(new_token);
    return NULL;
}
int change_directory(char** argv){
    if (argv[1] == NULL || argv[2] != NULL) {
        fprintf(stderr, "cd: Error Number of Parameters\n");
        return EXIT_FAILURE;
    }
    if(chdir(argv[1]) != 0) {
        perror("cd");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
int print_wd(){
    char cwd[MAX_COMMAND_LENGTH];
    if (getcwd(cwd, MAX_COMMAND_LENGTH) != NULL) {
        printf("%s\n", cwd);
        return EXIT_SUCCESS;
    } 
    else {
        perror("pwd");
        return EXIT_FAILURE;
    }
}
int which_d(char* fname){
    //char* path;
    char* temp = fname;
    fname = name_builder(fname);

    if(fname != NULL){
        printf("%s\n", fname);
        //free fname if it's a new path
        if(strcmp(fname,temp)!=0){
            free(fname); //path that was allocated in name_builder
        }

        return EXIT_SUCCESS;
    }
    if(strcmp(fname,temp)!=0){
        free(fname); //path that was allocated in name_builder
    }
    printf("mysh: invalid program.\n");
    return EXIT_FAILURE;
   
}


int execute_command(char *argv[], int input, int output) {
    //cd built in
    if (strcmp(argv[0], "cd") == 0) {
        return change_directory(argv);
        // cd command
    } 
    //pwd built in
    else if (strcmp(argv[0], "pwd") == 0) {
        if(output != STDOUT_FILENO){
            pid_t pid = fork();
            if (pid == -1) {
                perror("fork");
                return EXIT_FAILURE;
            }
            else if (pid == 0) {
                dup2(output, STDOUT_FILENO);
                close(output);
                //free each arg in argv
                int status = print_wd();
                for (int i = 0; argv[i]!=NULL; i++) {
                    free(argv[i]);
                }
                exit(status);
            }else{
                //parent
                int status;
                waitpid(pid, &status, 0);
                return status;
            }
        }
        return print_wd();
        // pwd command
    } 
    //which built in
    else if (strcmp(argv[0], "which") == 0){
        if (argv[1] == NULL || argv[2] != NULL) {
            fprintf(stderr, "which: Error Number of Parameters\n");
            return EXIT_FAILURE;
        }
        if(output != STDOUT_FILENO){
            pid_t pid = fork();
            if (pid == -1) {
                perror("fork");
                return EXIT_FAILURE;
            }
            else if (pid == 0) {
                dup2(output, STDOUT_FILENO);
                close(output);
                //free each arg in argv
                int status = which_d(argv[1]);
                for (int i = 0; argv[i]!=NULL; i++) {
                    free(argv[i]);
                }
                exit(status);
            }else{
                //parent
                int status;
                waitpid(pid, &status, 0);
                return status;
            }
        }
        return which_d(argv[1]);
    }
    // external command
    else {
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            return EXIT_FAILURE;
        }
        else if (pid == 0) {
            // child progress
            //set input and output redirects
            if(input != STDIN_FILENO){
                dup2(input, STDIN_FILENO);
                close(input);
            }
            if(output != STDOUT_FILENO){
                dup2(output, STDOUT_FILENO);
                close(output);
            }
            if (execv(argv[0], argv) == -1) {
                perror("execv");
                //free args
                for (int i = 0; argv[i]!=NULL; i++) {
                    free(argv[i]);
                }
                exit(EXIT_FAILURE);
            }
        }
        else {
            // father progress
            int status;
            waitpid(pid, &status, 0);
            return status;
        }
    }
    return EXIT_SUCCESS;
}

int execute_pipe_command(char *args1[], char *args2[], int in1, int out1, int in2, int out2) {
    //if process 1 has an output redirect, process 2 has no input
    //if process 2 has an input redirect, process 1's output is just lost
    int pipe_fds[2];
    pid_t pid1 = -1;
    pid_t pid2 = -1;
    int status;

    if (pipe(pipe_fds) == -1) {
        perror("pipe");
        return EXIT_FAILURE;
    }
    if (strcmp(args1[0], "cd") == 0) { //cd can't be forked b/c then it will only change dir of the child
        status = change_directory(args1);
    }else{
        pid1 = fork();
        if (pid1 == 0) {
            // first child
            //set up its redirects
            if(in1 != STDIN_FILENO){
                dup2(in1, STDIN_FILENO);
                close(in1);
            }
            if(out1 != STDOUT_FILENO){
                dup2(out1, STDOUT_FILENO);
                close(out1);
                //close both ends of pipe ?
                close(pipe_fds[0]); 
                close(pipe_fds[1]);
            }else{
                close(pipe_fds[0]); 
                dup2(pipe_fds[1], STDOUT_FILENO);   //Redirects standard out to the write side of the pipe
                close(pipe_fds[1]);
            }
            //catch built ins: pwd and which
            if(strcmp(args1[0], "pwd") == 0){
                status = print_wd();
                //free both args2 and args2
                for (int i = 0; args1[i]!=NULL; i++) {
                    free(args1[i]);
                }
                for (int i = 0; args2[i]!=NULL; i++) {
                    free(args2[i]);
                }
                exit(status);
                //exit(print_wd());
            }
            else if (strcmp(args1[0], "which") == 0){
                if (args1[1] == NULL || args1[2] != NULL) {
                    fprintf(stderr, "which: Error Number of Parameters\n");
                    for (int i = 0; args1[i]!=NULL; i++) {
                        free(args1[i]);
                    }
                    for (int i = 0; args2[i]!=NULL; i++) {
                        free(args2[i]);
                    }
                    exit(EXIT_FAILURE);
                }
                status = which_d(args1[1]);
                for (int i = 0; args1[i]!=NULL; i++) {
                    free(args1[i]);
                }
                for (int i = 0; args2[i]!=NULL; i++) {
                    free(args2[i]);
                }
                exit(status);
            }
            //external command:
            else{
                execv(args1[0], args1);
                perror("execv");
                for (int i = 0; args1[i]!=NULL; i++) {
                    free(args1[i]);
                }
                for (int i = 0; args2[i]!=NULL; i++) {
                    free(args2[i]);
                }
                exit(EXIT_FAILURE);
            }
            
        }
    }

    if (strcmp(args2[0], "cd") == 0) { //cd can't be forked b/c then it will only change dir of the child
        status = change_directory(args2);
    }else{
        pid2 = fork();
        if (pid2 == 0) {
            // second child
            //set up redirects
            if(out2 != STDOUT_FILENO){
                dup2(out2, STDOUT_FILENO);
                close(out2);
            }
            if(in2 != STDIN_FILENO){
                dup2(in2, STDIN_FILENO);
                close(in2);
                //close both ends of pipe ?
                close(pipe_fds[0]); 
                close(pipe_fds[1]);
            }else{
                close(pipe_fds[1]); 
                dup2(pipe_fds[0], STDIN_FILENO);  //Redirects standard input to the read side of the pipe
                close(pipe_fds[0]);
            }
            //catch built ins: pwd and which
            if(strcmp(args2[0], "pwd") == 0){
                status = print_wd();
                //free both args2 and args2
                for (int i = 0; args1[i]!=NULL; i++) {
                    free(args1[i]);
                }
                for (int i = 0; args2[i]!=NULL; i++) {
                    free(args2[i]);
                }
                exit(status);
            }
            else if (strcmp(args2[0], "which") == 0){
                if (args2[1] == NULL || args2[2] != NULL) {
                    fprintf(stderr, "which: Error Number of Parameters\n");
                    for (int i = 0; args1[i]!=NULL; i++) {
                        free(args1[i]);
                    }
                    for (int i = 0; args2[i]!=NULL; i++) {
                        free(args2[i]);
                    }
                    exit(EXIT_FAILURE);
                }
                status = which_d(args2[1]);
                for (int i = 0; args1[i]!=NULL; i++) {
                    free(args1[i]);
                }
                for (int i = 0; args2[i]!=NULL; i++) {
                    free(args2[i]);
                }
                exit(status);
                //exit(which_d(args2[1]));
            }
            //external command:
            else{
                execv(args2[0], args2);
                perror("execv");
                for (int i = 0; args1[i]!=NULL; i++) {
                    free(args1[i]);
                }
                for (int i = 0; args2[i]!=NULL; i++) {
                    free(args2[i]);
                }
                exit(EXIT_FAILURE);
            }
        }
    }

    // father
    close(pipe_fds[0]);
    close(pipe_fds[1]);
    if(pid1 != -1){
        waitpid(pid1, NULL, 0);
    }
    if(pid2 != -1){
        waitpid(pid2, &status, 0);
    }
   //only need status of second command (the one after the pipe)
    return status;

}

char *my_strdup(const char *s) {
    if (s == NULL) return NULL;

    size_t len = strlen(s) + 1; // +1 for the null terminator
    char *new_str = malloc(len);
    if (new_str) {
        memcpy(new_str, s, len); // Copy the string with the null terminator
    }
    return new_str;
}


void expand_wildcards(char *arg, char **argv, int *argc) {
    glob_t glob_result;
    memset(&glob_result, 0, sizeof(glob_result));

    // Use glob to expand wildcards. GLOB_NOCHECK returns the pattern itself if no matches are found.
    // GLOB_TILDE allows for tilde expansion for home directories.
    int result = glob(arg, GLOB_NOCHECK | GLOB_TILDE, NULL, &glob_result);

    if (result == 0) {
        for (int i = 0; i < glob_result.gl_pathc && *argc < MAX_ARGS - 1; i++) {
            // Duplicate each matched path and store in argv, incrementing argc.
            argv[*argc] = my_strdup(glob_result.gl_pathv[i]);
            (*argc)++;
        }
    }

    // Free the memory allocated by glob.
    globfree(&glob_result);
}

//need to change this for path/program names (its argv[0] but paths?), conditionals, and redirects
int parse_and_execute(char *command, int prev_status) {
    char *args[MAX_ARGS];           // Array to store command arguments
    char *args_pipe[MAX_ARGS];      // Array to store arguments after a pipe
    char *token = strtok(command, " "); // Tokenize the command string
    int argc = 0, argc_pipe = 0;    // Argument counters for both command parts
    int pipe_found = 0;             // Flag to check if a pipe is found

    int input = STDIN_FILENO;
    int output = STDOUT_FILENO;
    

    //check if first token is then or else
    if (strcmp(token, "then")==0) {
        if(prev_status != EXIT_SUCCESS){
            printf("mysh: error, conditional unsuccessful.\n");
            return EXIT_FAILURE;
        }
        token = strtok(NULL, " ");
    }else if(strcmp(token, "else")==0){
        if(prev_status == EXIT_SUCCESS){
            printf("mysh: error, conditional unsuccessful.\n");
            return EXIT_FAILURE;
        }
        token = strtok(NULL, " ");
        //read next token
    }

    //build program name/path
    char* temp = token; //later used to check if token was malloc'd or not
    token = name_builder(token);

    if( token == NULL){
        printf("mysh: invalid command.\n");
        return EXIT_FAILURE;
    }
    // Loop through tokens while the max argument count is not exceeded
    while (token != NULL && argc < MAX_ARGS - 1) {
        if (strcmp(token, "|") == 0) {
            pipe_found = 1;         // Set the pipe flag if a pipe is found
            break;
        } 
        else if (strchr(token, '*')) { //FINISH THIS
            // If a wildcard is found, expand it
            expand_wildcards(token, args, &argc);
        }
        else if (strchr(token, '<')){
            //input redirect
            token = strtok(NULL, " ");
            if( token == NULL){
                printf("error: didn't specify input redirect\n");
                return EXIT_FAILURE;
            }
            char* new_input = malloc(strlen(token)+1);
            strcpy(new_input, token);
            
            int fd = open(new_input, O_RDONLY);
            if (fd < 0) {
                perror(new_input);
                free(new_input);
                return EXIT_FAILURE;
            }
            input = fd;
            free(new_input);

        }else if (strchr(token, '>')){
            //output redirect
            token = strtok(NULL, " ");
            if( token == NULL){
                printf("error: didn't specify output redirect\n");
                return EXIT_FAILURE;
            }

            char* new_output = malloc(strlen(token)+1);
            strcpy(new_output, token);
            
            int fd = creat(new_output,S_IRUSR|S_IWUSR|S_IRGRP);
            if (fd < 0) {
                perror(new_output);
                free(new_output);
                return EXIT_FAILURE;
            }
            output = fd;
            free(new_output);
        }
        else {
            // Store the argument and increment the argument count
            args[argc] = malloc(MAX_COMMAND_LENGTH);
            strcpy(args[argc], token);
            if(argc==0 && strcmp(token,temp)!=0){
                free(token); //path that was allocated in name_builder
            }
            argc++;
        }
        token = strtok(NULL, " ");  // Get the next token
    }
    args[argc] = NULL; // Set the last element to NULL for exec

    if (pipe_found) { 
        int input2 = STDIN_FILENO;
        int output2 = STDOUT_FILENO;
        // If a pipe is found, process the arguments after the pipe

        char* token2 = strtok(NULL, " ");
        //build program name
        char* temp2 = token2; //later used to check if token was malloc'd or not 
        token2 = name_builder(token2);

        if( token2 != NULL){
            while (token2 != NULL && argc_pipe < MAX_ARGS - 1) {
                if (strchr(token2, '*')) { //FINISH THIS
                    // If a wildcard is found, expand it
                    expand_wildcards(token, args, &argc);
                }else if (strchr(token2, '<')){
                    //input redirect
                    token2 = strtok(NULL, " ");
                    if( token2 == NULL){
                        printf("error: didn't specify input redirect\n");
                        return EXIT_FAILURE;
                    }
                    char* new_input2 = malloc(strlen(token2)+1);
                    strcpy(new_input2, token2);
                    
                    int fd = open(new_input2, O_RDONLY);
                    if (fd < 0) {
                        perror(new_input2);
                        free(new_input2);
                        return EXIT_FAILURE;
                    }
                    input2 = fd;
                    free(new_input2);
                }else if (strchr(token2, '>')){
                    //output redirect
                    token2 = strtok(NULL, " ");
                    if( token2 == NULL){
                        printf("error: didn't specify output redirect\n");
                        return EXIT_FAILURE;
                    }
                    char* new_output2 = malloc(strlen(token2)+1);
                    strcpy(new_output2, token2);
                    
                    int fd = creat(new_output2,S_IRUSR|S_IWUSR|S_IRGRP);
                    if (fd < 0) {
                        perror(new_output2);
                        free(new_output2);
                        return EXIT_FAILURE;
                    }
                    output2 = fd;
                    free(new_output2);
                }
                else {
                    // Store the argument and increment the argument count
                    args_pipe[argc_pipe] = malloc(MAX_COMMAND_LENGTH);
                    strcpy(args_pipe[argc_pipe], token2);
                    if(argc_pipe==0 && strcmp(token2,temp2)!=0){
                        free(token2); //path that was allocated in name_builder
                    }
                    argc_pipe++;
                }
                token2 = strtok(NULL, " "); 
            }
            args_pipe[argc_pipe] = NULL; // Set the last element to NULL for exec

            prev_status = execute_pipe_command(args, args_pipe, input, output, input2, output2); // Execute the piped command

            // Free dynamically allocated memory for pipe arguments
            for (int i = 0; i < argc_pipe; i++) {
                free(args_pipe[i]);
            }
        }else{
            //pipe isn't followed by a valid command
            printf("mysh: invalid command following pipe.\n");
            prev_status = EXIT_FAILURE;
        }
        
        
    } else {
        prev_status = execute_command(args, input, output); // Execute the command if no pipe is found
    }

    // Free dynamically allocated memory for arguments
    for (int i = 0; i < argc; i++) {
        free(args[i]);
    }

    return prev_status;
}
void batch_mode(const char *filename) {
    char command[MAX_COMMAND_LENGTH];
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("open file error");
        exit(EXIT_FAILURE);
    }

    int prev_status =EXIT_SUCCESS;
    while (fgets(command, MAX_COMMAND_LENGTH, file)) {
        command[strcspn(command, "\n")] = 0;
        if (strcmp(command, "exit") == 0) {
            //printf("mysh: exiting\n");
            break;
        }
        prev_status = parse_and_execute(command, prev_status);
    }
    fclose(file);
}
void interactive_mode() {
  char command[MAX_COMMAND_LENGTH];

  printf("Welcome to my shell! Type 'exit' to quit\n");
  int prev_status = EXIT_SUCCESS;
  while (1) {
    printf("mysh> ");
    if (fgets(command, MAX_COMMAND_LENGTH, stdin) == NULL) {
      // if get wrong command
      printf("fgets() error\n");
      break;
    }
    if(command[0] == '\n'){//no stdin
        continue;
    }
    // remove line break
    command[strcspn(command, "\n")] = 0;

    // if input 'exit', then 'exit'
    if (strcmp(command, "exit") == 0) {
      break;
    }

    prev_status = parse_and_execute(command, prev_status);
  }
    printf("mysh: exiting..\n");
}




int main(int argc, char** argv){

    if (argc == 2) {
        // batch_mode
        batch_mode(argv[1]);
    } else {
        // interactive_mode first
        interactive_mode();
    }
    return EXIT_SUCCESS;
}