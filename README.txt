OVERVIEW:
    MyShell is a simple linux shell emulator. To run it 'cd' into '/MyShell' and do:
        % make clean
        % make
        % ./mysh
    
    MyShell DOES NOT support:
    	--tab completion
    	--consecutive piping (only single piping)
    	--command history (you won't be able to use the arrow keys to navigate previous commands)
    	--directory prompt (current directory will not be shown in prompt)

    MyShell DOES support:
    	--everything else :D

IMPLEMENTATION:
    We first set up argument arrays for both the current process, and a possible piped process, with respective counters. We also keep
    input and output variables to keep track of stdin and stdout for the current process.
    
    PARSING
        We split the entire command into tokens which are repeatedly read through strtok(). for each token we do the following:
        1. CONDITIONALS
            For only the first token, we check if it is either "else" or then". If so, we only continue if the previous status is correct.
        2. PROGRAM NAME VALIDITY
            Then, again only for the next token, we check if it is a valid program name by searching for it in the appropriate locations, using
            name_builder(). If so, we build the proper pathname and continue.
        3. PIPES, WILDCARDS, REDIRECTS, OR ARGUMENT
            Then, for every token thereafter, we check to see if it is one of 4 things:
                If it is a pipe, we stop parsing this process, and begin step 4.
                If it is a wildcard, we run expand_wildcard() and add every new argument to the argument list
                If it is an input or output redirect, we open the next token as a file, and update the input variable
                If it's neither of these things, it is an argument and is added to the argument list
        4. IF PIPE, REPEAT STEP 2 AND 3 FOR THE SECOND PROCESS

        After all of the parsing, the last argument of the argument list(s) is set to NULL. If the command is piped, execute_pipe_command() is run.
        Otherwise, execute_command() is run. This will return an exit staus which is returned to the caller, and passed back to the parser on 
        the next comamnd.
    EXECUTING
        EXECUTE_COMMAND
            The command can either be one fo the 3 built in commands or an external command

            If it is "cd", then we run change_directory()
            If it is "pwd":
                If there is an output redirect, we start a child process, set up the stdout, and run print_wd()
                Otherwise, we just run print_wd();
            If it is "which":
                We make sure there is one and only one argument.
                If there is an output redirect, we start a child process, set up the stdout, and run which_d()
                Otherwise, we just run which_d();
            If it's neither of these, it's an external command:
                A child process is created, and stdout and stdin is apprpriately set up.
                execv() is run

            After the command is executed, and the child (if any) is returned, the function returns EXIT_SUCCESS to the parsing function.

        EXECUTE_PIPE_COMMAND
            First a pipe is established. Then for the two processes, we do the following:
                If if the program is "cd", then we run change_directory().
                Otherwise we begin a child process and do the follwoingg:
                    We set the proper stdin and stdout using the pipe
                    Then, similar to execute_command() we check if the program is "pwd" or "which", and if so we run their respective functions.
                    Otherwise we run execv()
            Afterwards, the parent returns the status of the second process to the parsing functoin

    FREEING
        We free at the end of parse_and_execute() but also in the execute functions. If we ever run a  child process, we make sure to free the dynamically
        allocated arguments before exiting.
