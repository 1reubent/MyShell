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

//command parser
int command_parser(char* command, int exit_code){
    //Parse out:
        //conditionals (then/else)
            //then: previous exit must have been a success
            //else: previous exit must have been a fail
            //if either are false, just return, dont run command
        //path to exeutable file (process(es))
            //wildcards
        //list of arguments
            //store in string array, NULL TERMINATED! use array list
        //STDOUT AND IN
            //redirect: open input/output file, and save fd to pass to program_launcher

            //
    //Calling launcher:
        //normally, just call launcher with tokens
        //if there is a pipe
            //create pipe. then call launcher for each process with their respective components. then close the pipe (fd[0] and fd[1])
}
//program launcher
int program_launcher(char* path, char** args, int input_redirect, int output_redirect){
    if(fork() == 0){
        if(input_redirect!=NULL){
            dup2(input_redirect, STDIN_FILENO);
            close(input_redirect);
        }
        if(output_redirect!=NULL){
            dup2(output_redirect, STDOUT_FILENO);
            close(output_redirect);
        }
        execv(path, args);
        exit(1);
    }
    return 0;
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
        char* buf = malloc(BUFFER_SIZE * sizeof(char));
        int bytes;
        while((bytes = read(fd, buf, BUFFER_SIZE * sizeof(char))) > 0){
            

            //call parser_launcher
            //only pass everything up until the newline character. move everything after to front of buffer, and fill up buffer again for next command
        }
        if(bytes == -1){
            perror(argv[1]);
            exit(EXIT_FAILURE);
        }
        free(buf);
        close(fd);

        //
    }
    
    //0 arg
        //interactive mode. print greeting. print prompt "mysh>". wait for command, parse it, run it, save exit code, and repeat. end at exit or EOF
        //print exit message "exiting mysh"
    if(argc==1){

    }
}