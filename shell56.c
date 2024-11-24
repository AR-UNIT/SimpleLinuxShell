/*
 * file:        shell56.c
 * description: skeleton code for simple shell
 *
 * Peter Desnoyers, Northeastern CS5600 Fall 2024
 */

/* <> means don't check the local directory */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

/* "" means check the local directory */
#include "parser.h"

/* you'll need these includes later: */
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>

/* THIS IS WHAT I HAVE ADDED*/
#include <linux/limits.h>


/* if the token is cd, when we need to perform cd operation
check if the next token is something to be used by cd
*/
int cd_command(int argc, char *argv[]) {

  /* if the only token we have is cd, then we need to do cd into home directory
   */
  if (argc == 1) {
    if (chdir(getenv("HOME")) == -1) {
      printf("\n%s", strerror(errno));
      return 1;
    }
    chdir(getenv("HOME"));
    return 0;
  } else if (argc == 2) {
    if (chdir(argv[1]) == -1) {
      fprintf(stderr, "cd: %s\n", strerror(errno));
      return 1;
    } else {
      chdir(argv[1]);
      char path_buffer[PATH_MAX];
      getcwd(path_buffer, PATH_MAX);
      return 0;
    }
  } else {
    fprintf(stderr, "cd: wrong number of arguments\n");
    return 1;
  }
}


/* if the token is pwd, when we need to perform pwd operation,
  also, we need to asser that the number of tokens is 1 only for pwd
*/
int pwd_command(int argc, char *argv[]) {

  /* if the only token we have is cd, then we need to do cd into home directory
   */
  if (argc == 1) {
    char path_buffer[PATH_MAX];
    getcwd(path_buffer, PATH_MAX);
    printf("%s\n", path_buffer);
  } else {
    fprintf(stderr, "pwd: wrong number of arguments\n");
    return 1;
  }
  return 0;
}

/* if the token is exit, when we need to perform exit operation,
  we need to assert that the number of tokens is 1 only for pwd
*/
int exit_command(int argc, char *argv[], int status) {

  /* if the only token we have is cd, then we need to do cd into home directory
   */
  if (argc == 1) {
    exit(status);
    // return 0;
  } else if (argc == 2) {
    exit(atoi(argv[1]));
    // return 0;
  } else {
    fprintf(stderr, "exit: too many arguments\n");
    return 1;
  }
}

/*
    function to handle redirections, called from external_commands handler
   function
*/
int handle_redirections(int n_tokens, char *tokens[], int *saved_stdin,
                        int *saved_stdout) {
  /*
    save the input and output file descriptors for the shell
   */
  *saved_stdin = dup(STDIN_FILENO);
  *saved_stdout = dup(STDOUT_FILENO);

  /*
    initialize the new input and output file descriptors for redirect processing
   */
  char *input_file = NULL;
  char *output_file = NULL;

  /*
  iterate through tokens to identify redirect operators
   */
  for (int i = 0; i < n_tokens; i++) {
    if (strcmp(tokens[i], ">") == 0) {
      /*
      if output redirect, fetch the file to output to
       */

      if (i + 1 < n_tokens) {
        output_file = tokens[i + 1];
        /*
        terminate the command at the redirection operator by setting null
         */
        tokens[i] = NULL;
      } else {
        fprintf(stderr, "Error: File not found for output redirection.\n");
        return 1;
      }
    } else if (strcmp(tokens[i], "<") == 0) {
      /*
          if input redirect, fetch the file to output to
       */
      if (i + 1 < n_tokens) {
        input_file = tokens[i + 1];
        /*
        terminate the command at the redirection operator by setting null
         */
        tokens[i] = NULL;
      } else {
        fprintf(stderr, "Error: File not found for input redirection.\n");
        return 1;
      }
    }
  }

  if (output_file) {
    int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
      fprintf(stderr, "unable to open file.\n");
      return 1;
    }
    int res = dup2(fd, STDOUT_FILENO);
    if (res == -1) {
      fprintf(stderr, "unable to point output fd of shell to fd of destination.\n");
      close(fd);
      return 1;
    }
    close(fd);
  }

  if (input_file) {
    int fd = open(input_file, O_RDONLY);
    if (fd == -1) {
      fprintf(stderr, "unable to open file.\n");
      return 1;
    }
    if (dup2(fd, STDIN_FILENO) == -1) {
      fprintf(stderr, "unable to point input fd of shell to fd of source.\n");
      close(fd);
      return 1;
    }
    close(fd);
  }

  return 0;
}


bool bad_operators_in_command(int argc, char *argv[]) {
    int operators = 0;
    int commands = 0;

    for (size_t i = 0; i < argc; i++) {

        /* if operators present check that they are not at edges and previous token was also not an operator*/
        if (strcmp(argv[i], "|") == 0 || strcmp(argv[i], ">") == 0 || strcmp(argv[i], "<") == 0) {
            if (i == 0 || i == argc - 1 || (i > 0 && (strcmp(argv[i - 1], "|") == 0 || strcmp(argv[i - 1], ">") == 0 || strcmp(argv[i - 1], "<") == 0))) {
                return true;
            }
            operators += 1;
        } else {
            commands += 1;
        }
    }

    if (operators != (commands - 1) ) {
        return false;
    }
    return true;
}

int handle_pipes(int argc, char *argv[], int *saved_stdin, int *saved_stdout) {
    if (bad_operators_in_command(argc, argv)) {
        return 1;
    }

    int pipe_fds[2];
    pid_t pid;
    int prev_fd = -1;


    /* saving the fds for standard input and output of shell */
    *saved_stdin = dup(STDIN_FILENO);
    *saved_stdout = dup(STDOUT_FILENO);

    /* iterating throught the shell to excecte piped commands  */
    for (int i = 0; i < argc;) {
        int cmd_start = i;
        while (i < argc && strcmp(argv[i], "|") != 0) {
            i++;
        }
        /* null terminating piped value */
        argv[i] = NULL;

        if (i < argc) {
            if (pipe(pipe_fds) == -1) {
                fprintf(stderr, "pipe call failed: %s\n", strerror(errno));
                return 1;
            }
        }

        pid = fork();
        if (pid == -1) {
            fprintf(stderr, "Fork failed: %s\n", strerror(errno));
            return 1;
        } else if (pid == 0) {
            /* inside the child process */
            /* reset signal handling to allow ^C to interrupt in child processes*/
            signal(SIGINT, SIG_DFL);

            /* prev_fd tracks the output fd of the previously executed command,
               if prev_fd != -1, then at least one command has been executed previously */
            if (prev_fd != -1) {
              /* setting standard input of current process to be the output of the previous process */
                dup2(prev_fd, STDIN_FILENO);
                close(prev_fd);
            }

            /* if the current token/command is not the last, then we need to set standard output of
                the current command to go to the the output fd in pipe  */
            if (i < argc) {
                dup2(pipe_fds[1], STDOUT_FILENO);
                /* now the read end of pipe may be accessed in next command to get input */
            }


            /* close the write and read ends of pipe in child */
            close(pipe_fds[1]);
            close(pipe_fds[0]);

            /* after setting up the fds for input of current command, and output flow of current
                command, we can finally execute the current command by fork and execute */
            int status = fork_and_execute_external_command(&argv[cmd_start]);
            if (status == -1) {
                exit(EXIT_FAILURE);
            }
            exit(EXIT_SUCCESS);
        } else {
          /* close write end of the parent */
            close(pipe_fds[1]);

            /* close the (read end of pipe / input fd of the previously executed command) */
            if (prev_fd != -1) {
                close(prev_fd);
            }

            /* save the (read end of pipe / output fd of this command) for the next command */
            prev_fd = pipe_fds[0];
        }

        i++;
    }

    /* close the final read end in the parent process */
    if (prev_fd != -1) {
        close(prev_fd);
    }

    /* parent waits for children to finish processing*/
    while (wait(NULL) > 0);

    /* restore previous fds for the shell */
    dup2(*saved_stdin, STDIN_FILENO);
    dup2(*saved_stdout, STDOUT_FILENO);
    close(*saved_stdin);
    close(*saved_stdout);
    return 0;
}




/*
function to fork and execute command in subprocess, accepts tokens array
 */
int fork_and_execute_external_command(char *argv[]) {
  int pid = fork();

  /**
   * all code that is written outside of pid if else check and below fork is
   * printing values from main pr
   */
  if (pid == 0) {
    /* inside the child process,
    reset default interrupt signal ^C inside child process
    */
    signal(SIGINT, SIG_DFL);
    int status = execvp(argv[0], argv);
    /* If execvp fails, print error and exit
     */
    printf("%s: %s\n", argv[0], strerror(errno));
    exit(EXIT_FAILURE);
  }

  else if (pid == -1) {
    /* error has ocurred, print error and exit
     */
    fprintf(stderr, "Fork failed: %s\n", strerror(errno));
  }
  return pid;
}

/*
handler function to execute all external commands
 */
int external_command(int argc, char *argv[]) {
  int saved_stdin, saved_stdout;

  /*
    checking if the command has redirection by iterating through the tokens
   */
  bool has_redirection = false;
  bool has_pipes = false;
  for (int i = 0; i < argc; i++) {
    if (strcmp(argv[i], ">") == 0 || strcmp(argv[i], "<") == 0) {
      has_redirection = true;
      break;
    }
    if (strcmp(argv[i], "|") == 0) {
      has_pipes = true;
      break;
    }
  }

  if (has_redirection) {
    if (handle_redirections(argc, argv, &saved_stdin, &saved_stdout) != 0) {
      return 1;
    }
  } else if (has_pipes){
    if (handle_pipes(argc, argv, &saved_stdin, &saved_stdout) != 0) {
      return 1;
    }
    else {
      return 0;
    }
  }
    else {
    saved_stdin = dup(STDIN_FILENO);
    saved_stdout = dup(STDOUT_FILENO);
  }

  /* at least one argument for external command */
  if (argc == 0){
    return 1;
  }

  int pid = fork_and_execute_external_command(argv);

  if (pid == -1) {
    /*
    error returned from forked child process,
    -1 returned by default to pid if forked child errors out,
    fork_and_execute_external_command checks for the error == -1,
    prints error message that child process has errored out,
    and then returns -1 to be consumed by this function
     */
    return 1;
  }

  int status;
  do {
    waitpid(pid, &status, WUNTRACED);
  } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  int exit_status = WEXITSTATUS(status);

  /*
  restore original file descriptors of shell
   */
  dup2(saved_stdin, STDIN_FILENO);
  dup2(saved_stdout, STDOUT_FILENO);
  close(saved_stdin);
  close(saved_stdout);

  return exit_status;
}

int main(int argc, char **argv) {
  int status = 0;
  bool interactive = isatty(STDIN_FILENO); /* see: man 3 isatty */
  FILE *fp = stdin;

  if (argc == 2) {
    interactive = false;
    fp = fopen(argv[1], "r");
    if (fp == NULL) {
      fprintf(stderr, "%s: %s\n", argv[1], strerror(errno));
      exit(EXIT_FAILURE); /* see: man 3 exit */
    }
  }
  if (argc > 2) {
    fprintf(stderr, "%s: too many arguments\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  char line[1024], linebuf[1024];
  const int max_tokens = 32;
  char *tokens[max_tokens];

  /* loop:
   *   if interactive: print prompt
   *   read line, break if end of file
   *   tokenize it
   *   print it out <-- your logic goes here
   */
  while (true) {
    signal(SIGINT, SIG_IGN); /* ignore SIGINT=^C */
    if (interactive) {
      /* print prompt. flush stdout, since normally the tty driver doesn't
       * do this until it sees '\n'
       */
      printf("$ ");
      fflush(stdout);
    }



    char qbuf[16];

    /* see: man 3 fgets (fgets returns NULL on end of file)
     */
    if (!fgets(line, sizeof(line), fp)) break;

    /* read a line, tokenize it, and print it out
     */
    int n_tokens = parse(line, max_tokens, tokens, linebuf, sizeof(linebuf));

    /* replace the code below with your shell:
     */
    // for (int i = 0; i < n_tokens; i++){
    //     printf(" '%s'", tokens[i]);
    // }

    /* this keeps the shell from crashing if the input has 0 tokens. (\n)
     */
    if (n_tokens == 0) {
      continue;
    }

    /* if the command is a cd command, the 1st token(0-index) has to be cd,
        and the min number of tokens possible is 1 including cd
        and the max number of tokens possible is 2 including cd
    */
    if (strcmp("cd", tokens[0]) == 0) {
      status = cd_command(n_tokens, tokens);
    }

    else if (strcmp("pwd", tokens[0]) == 0) {
      status = pwd_command(n_tokens, tokens);
    }

    else if (strcmp("exit", tokens[0]) == 0) {
      status = exit_command(n_tokens, tokens, status);
    }

    else if (strcmp("$?", tokens[0]) == 0 ||
        (n_tokens == 2 && strcmp("echo", tokens[0]) == 0 && strcmp("$?", tokens[1]) == 0)) {
        /* implementing $? and echo $? support */
        for (int i = 0; i < n_tokens; i++) {
            if (strcmp("$?", tokens[i]) == 0) {
                tokens[i] = qbuf;
                printf("%s\n", tokens[i]);
            }
        }
    }

    else {
      status = external_command(n_tokens, tokens);
    }

    sprintf(qbuf, "%d", status);
  }

  if (interactive) /* make things pretty */
    printf("\n");  /* try deleting this and then quit with ^D */

  return status;
}
