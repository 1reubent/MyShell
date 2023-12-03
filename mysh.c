#include <stdio.h> 
#include <stdlib.h> 
#include <stdbool.h>
#include <unistd.h>
#include <string.h> 

#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/wait.h>

#include "arraylist.h"
#define MAX_COMMAND_LENGTH 32 //assume no commands get cut off
#define BATCH 0
#define INTER 1
#define MAX_ARGS 12

//command parser
// int command_parser(int fd, char* fname, int mode){
    
//     //READING, PARSING, and LAUNCHING COMMAND
//     char* buf = malloc(BUFFER_SIZE * sizeof(char));
//     int bytes;
//     int pipe_flag = 0; //flag if there is a pipe
//     int prev_status = EXIT_SUCCESS;
//     while((bytes = read(fd, buf, BUFFER_SIZE * sizeof(char))) > 0){
//         //READING
//         //only READ everything up until the newline character.
//         //if buffer isn big enough for the full command, parse what you have,move partial tokens to front, flush buffer, and continue (dont go to launching)
        
//         //PARSING
//             //conditionals (then/else)
//                 //then: previous exit must have been a success
//                 //else: previous exit must have been a fail
//                 //if either are false, just return, dont run command
//         if( (strncmp(buf, "then", 4) && prev_status != EXIT_SUCCESS) || (strncmp(buf, "else", 4) && prev_status == EXIT_SUCCESS) ){
//             //read until newline, save partial tokens, and continue to next command
//         } 

//         for(int i=0; i< BUFFER_SIZE; i++){
//             //path to exeutable file (process(es))
//                 //wildcards
//             //list of arguments
//                 //store in string array, NULL TERMINATED! use array list
//             //STDOUT AND IN
//                 //redirect: open input/output file, and save fd to pass to program_launcher
//                 //pipe:  pipe=1, store the second process' path/args/redirects as well

//             //return -1 if it encounters exit command

//         }
        

//         //LAUNCHING
//         //normally, just call launcher with tokens
//         //if there is a pipe (pipe_flag is true)
//             //create pipe. then call launcher for each process with their respective components. then close the pipe (fd[0] and fd[1])
//         //store status for later use
//         if(pipe_flag){ 
//             int fdp[2];
//             pipe(fdp);
//             program_launcher(/* tokens*/); //run first process
//             close(fdp[1]);

//         }
//         prev_status = program_launcher(/*othe tokens*/); //run second OR only process
//         if(mode == INTER){
//             printf("myshell> ");
//         }
//     //finished current command
//     }
//     if(bytes == -1){
//         perror(fname);
//         exit(EXIT_FAILURE);
//     }
//     free(buf);
//     return 0;

    

// }
// //program launcher
// int program_launcher(char* path, char** args, int input_redirect, int output_redirect){
//     if(fork() == 0){
//         //child
//         if(input_redirect!=NULL){
//             dup2(input_redirect, STDIN_FILENO);
//         }
//         if(output_redirect!=NULL){
//             dup2(output_redirect, STDOUT_FILENO);
//         }
//         execv(path, args);
//         exit(1);
//     }
//     //parent
//     int child_status;
//     wait(&child_status);
//     return child_status;
// }
int execute_command(char *argv[], int input, int output) {
    if (strcmp(argv[0], "cd") == 0) {
        // cd command
        if (argv[1] == NULL || argv[2] != NULL) {
            fprintf(stderr, "cd: Error Number of Parameters\n");
            return EXIT_FAILURE;
        } else if (chdir(argv[1]) != 0) {
            perror("cd");
            return EXIT_FAILURE;
        }
    } 
    else if (strcmp(argv[0], "pwd") == 0) {
        // pwd command
        char cwd[MAX_COMMAND_LENGTH];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s\n", cwd);
        } 
        else {
            perror("pwd");
            return EXIT_FAILURE;
        }
    } 
    //ALSO IMPLEMENT WHICH COMMAND
    else {
        // external command
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
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
                return EXIT_FAILURE;
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
    pid_t pid1, pid2;

    if (pipe(pipe_fds) == -1) {
        perror("pipe");
        return EXIT_FAILURE;
    }

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
        execv(args1[0], args1);
        perror("execv");
        return EXIT_FAILURE;
    }

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

        execv(args2[0], args2);
        perror("execv");
        return EXIT_FAILURE;
    }

    // father
    close(pipe_fds[0]);
    close(pipe_fds[1]);

    int status2;
    waitpid(pid1, NULL, 0);
    waitpid(pid2, &status2, 0);
    return status2;


}
char* name_builder(char* token){
    //pathname OR built-in: cd, pwd, which
    if(strchr(token, '/')!= NULL || strcmp(token, "cd") ==0 || strcmp(token, "pwd") ==0 || strcmp(token, "which") ==0){
        return token;
    }
    //bare name. search for it
    //char path[14+strlen(token)];
    char* path = malloc(14+strlen(token));
    strcpy(path,"/usr/local/bin");
    strcat(path, token);
    if(access(path, F_OK)){
        return path;
    }
    memset(path,0,strlen(path));
    strcpy(path,"/usr/bin");
    strcat(path, token);
    if(access(path, F_OK)){
        return path;
    }
    memset(path,0,strlen(path));
    strcpy(path,"/bin");
    strcat(path, token);
    if(access(path, F_OK)){
        return path;
    }
    return NULL;
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
    if ((strcmp(token, "then")==0 && prev_status != EXIT_SUCCESS) || (strcmp(token, "else")==0 && prev_status == EXIT_SUCCESS)) {
      printf("mysh: error, conditional unsuccessful. continuing..\n");
      return EXIT_FAILURE;
    }

    //build program name/path
    if( (token = name_builder(token)) == NULL){
        printf("mysh: invalid command. continuing...");
        return EXIT_FAILURE;
    }
    token = strtok(NULL, " ");
    // Loop through tokens while the max argument count is not exceeded
    while (token != NULL && argc < MAX_ARGS - 1) {
        if (strcmp(token, "|") == 0) {
            pipe_found = 1;         // Set the pipe flag if a pipe is found
            //SET OUTPUT OF FIRST PROCESS
            break;
        } 
        else if (strchr(token, '*')) {
            // If a wildcard is found, expand it
            //expand_wildcards(token, args, &argc);
        }
        else if (strchr(token, '<')){
            //input redirect
            char* new_input = malloc(strlen(token)-1);
            //char new_input[strlen(token)-1];
            strcpy(new_input, token+1);
            
            int fd = open(new_input, O_RDONLY);
            if (fd < 0) {
                printf("ERROR\n");
                perror(new_input);
                return EXIT_FAILURE;
            }
            input = fd;
            //dup2 to STDFILEIN in execute_command
            //not inlcluded in argumanet list

        }else if (strchr(token, '>')){
            //output redirect
            char* new_output = malloc(strlen(token)-1);
            //char new_output[strlen(token)-1];
            strcpy(new_output, token+1);
            
            int fd = creat(new_output,S_IRUSR|S_IWUSR|S_IRGRP);
            if (fd < 0) {
                printf("ERROR\n");
                perror(new_output);
                return EXIT_FAILURE;
            }
            output = fd;
        }
        else {
            // Store the argument and increment the argument count
            strcpy(args[argc], token);
            argc++;
            //args[argc++] = strdup(token);
        }
        token = strtok(NULL, " ");  // Get the next token
    }
    args[argc] = NULL;              // Set the last element to NULL for exec

    if (pipe_found) { //REDIRECTS
        int input2 = STDIN_FILENO;
        int output2 = STDOUT_FILENO;
        // If a pipe is found, process the arguments after the pipe

        //SET INPUT OF SECOND PROCESS

        //build program name
        token = strtok(NULL, " ");
        while (token != NULL && argc_pipe < MAX_ARGS - 1) {
            if (strchr(token, '*')) {
                // If a wildcard is found, expand it
                //expand_wildcards(token, args, &argc);
            }else if (strchr(token, '<')){
                //input redirect
                char* new_input = malloc(strlen(token)-1);
                //char new_input[strlen(token)-1];
                strcpy(new_input, token+1);
                
                int fd = open(new_input, O_RDONLY);
                if (fd < 0) {
                    printf("ERROR\n");
                    perror(new_input);
                    return EXIT_FAILURE;
                }
                input2 = fd;
                //dup2 to STDFILEIN in execute_command
                //not inlcluded in argumanet list

            }else if (strchr(token, '>')){
                //output redirect
                char* new_output = malloc(strlen(token)-1);
                strcpy(new_output, token+1);
                
                int fd = creat(new_output,S_IRUSR|S_IWUSR|S_IRGRP);
                if (fd < 0) {
                    printf("ERROR\n");
                    perror(new_output);
                    return EXIT_FAILURE;
                }
                output2 = fd;
            }
            else {
                // Store the argument and increment the argument count
                strcpy(args_pipe[argc_pipe], token);
                argc_pipe++;
                //args_pipe[argc_pipe++] = strdup(token);
            }
            token = strtok(NULL, " "); 
        }
        args_pipe[argc_pipe] = NULL; // Set the last element to NULL for exec

        prev_status = execute_pipe_command(args, args_pipe, input, output, input2, output2); // Execute the piped command

        // Free dynamically allocated memory for pipe arguments
        for (int i = 0; i < argc_pipe; i++) {
            free(args_pipe[i]);
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
        printf("mysh: exiting\n");
        break;
    }
    prev_status = parse_and_execute(command, prev_status);
  }

  fclose(file);
}
void interactive_mode() {
  char command[MAX_COMMAND_LENGTH];

  printf("Welcome to my shell!\n");
  int prev_status = EXIT_SUCCESS;
  while (1) {
    printf("mysh> ");
    if (!fgets(command, MAX_COMMAND_LENGTH, stdin)) {
      // if get wrong command
      break;
    }

    // remove line break
    command[strcspn(command, "\n")] = 0;

    // if input 'exit', then 'exit'
    if (strcmp(command, "exit") == 0) {
      printf("mysh: exiting\n");
      break;
    }

    prev_status = parse_and_execute(command, prev_status);
  }
}




int main(int argc, char** argv){

    if (argc == 2) {
        // batch_mode
        batch_mode(argv[1]);
    } else {
        // interactive_mode first
        interactive_mode();
    }
    return 0;
    //1 arg
    //     //batch mode. run each line/command as a child process, saving the exit codes. finish when you encounter exit, or EOF
    // if(argc==2){
    //     int fd = open(argv[1], O_RDONLY);
    //     if (fd < 0) {
    //         printf("ERROR\n");
    //         perror(argv[1]);
    //         exit(EXIT_FAILURE);
    //     }
    //     command_parser(fd, argv[1], BATCH);
    //     close(fd);
    //     //
    // }
    
    // //0 arg
    //     //interactive mode. print greeting. print prompt "mysh>". wait for command, parse it, run it, save exit code, and repeat. end at exit or EOF
    //     //print exit message "exiting mysh"
    // if(argc==1){
    //     printf("Welcome to my shell!\n");
    //     printf("myshell> ");
    //     command_parser(STDIN_FILENO, "STDIN", INTER);
    //     printf("existing myshell\n");
    // }
    return EXIT_SUCCESS;
}