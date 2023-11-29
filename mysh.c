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
#define BUFFER_SIZE 32
#define BATCH 0
#define INTER 1

//command parser
int command_parser(int fd, char* fname, int mode){
    
    //READING, PARSING, and LAUNCHING COMMAND
    char* buf = malloc(BUFFER_SIZE * sizeof(char));
    int bytes;
    int pipe_flag = 0; //flag if there is a pipe
    int prev_status = EXIT_SUCCESS;
    while((bytes = read(fd, buf, BUFFER_SIZE * sizeof(char))) > 0){
        //READING
        //only READ everything up until the newline character.
        //if buffer isn big enough for the full command, parse what you have,move partial tokens to front, flush buffer, and continue (dont go to launching)
        
        //PARSING
            //conditionals (then/else)
                //then: previous exit must have been a success
                //else: previous exit must have been a fail
                //if either are false, just return, dont run command
        if( (strncmp(buf, "then", 4) && prev_status != EXIT_SUCCESS) || (strncmp(buf, "else", 4) && prev_status == EXIT_SUCCESS) ){
            //read until newline, save partial tokens, and continue to next command
        } 

        for(int i=0; i< BUFFER_SIZE; i++){
            //path to exeutable file (process(es))
                //wildcards
            //list of arguments
                //store in string array, NULL TERMINATED! use array list
            //STDOUT AND IN
                //redirect: open input/output file, and save fd to pass to program_launcher
                //pipe:  pipe=1, store the second process' path/args/redirects as well

            //return -1 if it encounters exit command

        }
        

        //LAUNCHING
        //normally, just call launcher with tokens
        //if there is a pipe (pipe_flag is true)
            //create pipe. then call launcher for each process with their respective components. then close the pipe (fd[0] and fd[1])
        //store status for later use
        if(pipe_flag){ 
            int fdp[2];
            pipe(fdp);
            program_launcher(/* tokens*/); //run first process
            close(fdp[1]);

        }
        prev_status = program_launcher(/*othe tokens*/); //run second OR only process
        if(mode == INTER){
            printf("myshell> ");
        }
    //finished current command
    }
    if(bytes == -1){
        perror(fname);
        exit(EXIT_FAILURE);
    }
    free(buf);
    return 0;

    

}
//program launcher
int program_launcher(char* path, char** args, int input_redirect, int output_redirect){
    if(fork() == 0){
        //child
        if(input_redirect!=NULL){
            dup2(input_redirect, STDIN_FILENO);
        }
        if(output_redirect!=NULL){
            dup2(output_redirect, STDOUT_FILENO);
        }
        execv(path, args);
        exit(1);
    }
    //parent
    int child_status;
    wait(&child_status);
    return child_status;
}



int main(int argc, char** argv){
    //1 arg
        //batch mode. run each line/command as a child process, saving the exit codes. finish when you encounter exit, or EOF
    if(argc==2){
        int fd = open(argv[1], O_RDONLY);
        if (fd < 0) {
            printf("ERROR\n");
            perror(argv[1]);
            exit(EXIT_FAILURE);
        }
        command_parser(fd, argv[1], BATCH);
        close(fd);
        //
    }
    
    //0 arg
        //interactive mode. print greeting. print prompt "mysh>". wait for command, parse it, run it, save exit code, and repeat. end at exit or EOF
        //print exit message "exiting mysh"
    if(argc==1){
        printf("Welcome to my shell!\n");
        printf("myshell> ");
        command_parser(STDIN_FILENO, "STDIN", INTER);
        printf("existing myshell\n");
    }
    return EXIT_SUCCESS;
}