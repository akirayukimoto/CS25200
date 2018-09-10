// CS252: Shell project
//
// Template file.
// You will need to add more code here to execute the command table.
// You also will probably want to add other files as you add more functionality,
// unless you like having one massive file with thousands of lines of code!
//
// NOTE: You are responsible for fixing any bugs this code may have!
//

#include "command.h"

#include <signal.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

#include "y.tab.h"

// These global variables are used by shell.y to
// keep track of the current parser state.

command *current_command;
simple_command *current_simple_command;

/*
 * Allocate and initialize a new simple_command.
 * Return a pointer to it.
 */

simple_command *simple_command_create() {
  // Initially allocate space for this many arguments

#define INITIAL_ARUMENT_ARR_SIZE (5)

  simple_command *new_simple_command =
    (simple_command *) malloc(sizeof(simple_command));

  // Create initial space for arguments (may need expansion later)

  new_simple_command->num_available_arguments = INITIAL_ARUMENT_ARR_SIZE;
  new_simple_command->num_arguments = 0;
  new_simple_command->arguments = (char **)
    malloc(new_simple_command->num_available_arguments * sizeof(char *));

  return new_simple_command;
} /* simple_command_create() */

/*
 * Insert an argument into a simple_command.
 * Dynamically expand the argument array if it is too small.
 */

void simple_command_insert_argument(simple_command *s_command, char *argument) {
  if (s_command->num_available_arguments == s_command->num_arguments + 1) {
    // Double the available space

    s_command->num_available_arguments *= 2;
    s_command->arguments =
      (char **) realloc(s_command->arguments,
                        s_command->num_available_arguments * sizeof(char *));
  }

  s_command->arguments[s_command->num_arguments] = argument;

  // NULL argument signals end of arguement list

  s_command->arguments[s_command->num_arguments + 1] = NULL;

  s_command->num_arguments++;
} /* simple_command_insert_argument() */

/*
 * Allocate and initialize a new command. Return a pointer to it.
 */

command *command_create() {
  command *new_command = (command *) malloc(sizeof(command));

  // Create space to point to the first simple command

  new_command->num_available_simple_commands = 1;
  new_command->simple_commands =
    (simple_command **) malloc(new_command->num_available_simple_commands *
                               sizeof(simple_command *));

  new_command->num_simple_commands = 0;
  new_command->out_file = 0;
  new_command->in_file = 0;
  new_command->err_file = 0;
  new_command->is_background = 0;
  new_command->is_appended = 0;
  new_command->outfile_counter = 0;
  new_command->is_cleared = 1;
  new_command->last_pid = 0;
  new_command->last_return = 0;

  return new_command;
} /* command_create() */

/*
 * Insert a simple_command into the command. If we don't have enough
 * space in simple_commands, dynamically allocate more space.
 */

void command_insert_simple_command(command *command,
                                   simple_command *s_command) {
  if (command->num_available_simple_commands == command->num_simple_commands) {
    // double size of simple command list

    command->num_available_simple_commands *= 2;
    command->simple_commands = (simple_command **)
       realloc(command->simple_commands,
               command->num_available_simple_commands *
               sizeof(simple_command *));
  }

  command->simple_commands[command->num_simple_commands] = s_command;
  command->num_simple_commands++;
} /* command_insert_simple_command() */

/*
 * Completely clear a command, freeing all struct members.
 * After running, the command will be completely ready, as if
 * it was newly created.
 */

void command_clear(command *command) {
  for (int i = 0; i < command->num_simple_commands; i++) {
    for (int j = 0; j < command->simple_commands[i]->num_arguments; j++) {
      free(command->simple_commands[i]->arguments[j]);
      command->simple_commands[i]->arguments[j] = NULL;
    }

    free(command->simple_commands[i]->arguments);
    command->simple_commands[i]->arguments = NULL;

    free(command->simple_commands[i]);
    command->simple_commands[i] = NULL;
  }

  command->num_simple_commands = 0;

  if (command->out_file) {
    free(command->out_file);
    command->out_file = NULL;
  }

  if (command->in_file) {
    free(command->in_file);
    command->in_file = NULL;
  }

  if (command->err_file) {
    free(command->err_file);
    command->err_file = NULL;
  }

  command->is_background = 0;
  command->is_appended = 0;
  command->outfile_counter = 0;
  command->is_cleared = 1;
  command->last_pid = 0;
  command->last_return = 0;

} /* command_clear */

/*
 * Print the command table for a command.
 * This displays in a human-readable format, all simple_commands
 * in the command, and other metadata in the command including
 * input redirection and background status.
 */

void command_print(command *command) {
  printf("\n\n");
  printf("              COMMAND TABLE                \n");
  printf("\n");
  printf("  #   Simple Commands\n");
  printf("  --- ----------------------------------------------------------\n");

  for (int i = 0; i < command->num_simple_commands; i++) {
    printf("  %-3d ", i);
    for (int j = 0; j < command->simple_commands[i]->num_arguments; j++) {
      printf("\"%s\" \t", command->simple_commands[i]->arguments[j]);
    }
  }

  printf("\n\n");
  printf("  Output       Input        Error        Background\n");
  printf("  ------------ ------------ ------------ ------------\n");
  printf("  %-12s %-12s %-12s %-12s\n",
         command->out_file ? command->out_file : "default",
         command->in_file ? command->in_file : "default",
         command->err_file ? command->err_file : "default",
         command->is_background ? "YES" : "NO");
  printf("\n\n");
} /* command_print() */

/*
 * Execute the command, setting up all input redirection as specified.
 * Current this is basically a no-op. You must make it work.
 */

void command_execute(command *command) {
  // Don't do anything if there are no simple commands

  if ((command->num_simple_commands == 0)) {
    prompt();
    return;
  }

  // Print contents of Command data structure

  // Add execution here
  // For every simple command fork a new process
  // Setup i/o redirection
  // and call exec

  if (!strcmp(command->simple_commands[0]->arguments[0], "exit")){
    command_clear(command);
    printf(" Good bye!!\n");
    fflush(stdout);
    exit(0);
  }

  //command_print(command);
  if (command->outfile_counter > 1) {
    printf("Ambiguous output redirect.\n");
  }

  // save in/out/error files
  int tmpin = dup(0);
  int tmpout = dup(1);
  int tmperr = dup(2);

  // set the initial input
  int fdin;
  if (command->in_file != NULL) {
    fdin = open(command->in_file, O_RDONLY, 0664);
  }
  else {
    // use the default input file
    fdin = dup(tmpin);
  }

  int ret;
  int fdout;
  int fderr;
  int i;
  for (i = 0; i < command->num_simple_commands; i++) {
    // Redirect input file
//    dup2(fdin, 0);
//    close(fdin);

    if (!strcmp((command->simple_commands[i]->arguments[0]), "exit")) {
      printf(" Good bye!!\n");
      fflush(stdout);
      command_clear(command);
      exit(0);
    }

    dup2(fdin, 0);
    close(fdin);

    // When the command is the last command
    if (i == command->num_simple_commands - 1) {
      // setup the error file and redirect it
      if (command->err_file != NULL) {
        if (command->is_appended == 1) {
          fderr = open(command->err_file, O_CREAT|O_WRONLY|O_APPEND, 0600);
        }
        else {
          fderr = open(command->err_file, O_CREAT|O_WRONLY|O_APPEND, 0600);
        }
      }
      else {
        fderr = dup(tmperr);
      }

      dup2(fderr, 2);
      close(fderr);

      // setup the output file
      if (command->out_file != NULL) {
        if (command->is_appended == 1) {
          fdout = open(command->out_file, O_CREAT|O_WRONLY|O_APPEND, 0600);
        }
        else {
          fdout = open(command->out_file, O_CREAT|O_WRONLY|O_TRUNC, 0600);
        }
      }
      else {
        // use the default output file
        fdout = dup(tmpout);
      }
    }
    else {
      // since this is not the last simple command
      // we create pipe

      int fdpipe[2];
      if (pipe(fdpipe) < 0) {
        exit(1);
      }

      fdout = fdpipe[1];
      fdin = fdpipe[0];
    }
    // redirect output
    dup2(fdout, 1);
    close(fdout);

    if (!strcmp(command->simple_commands[i]->arguments[0], "cd")) {
      int dir;
      if (command->simple_commands[i]->num_arguments > 1) {
        dir = chdir(command->simple_commands[i]->arguments[1]);
      }
      else {
        dir = chdir(getenv("HOME"));
      }

      if (dir < 0) {
        //fprintf(stderr, "%s\n", command->simple_commands[i]->arguments[1]);
        char err_msg[1024];
        char *dir_name = command->simple_commands[i]->arguments[1];
        //strcat(err_msg, dir_name);
        sprintf(err_msg, "cd: %s", dir_name);
        perror(err_msg);
        //int error = dup(tmperr);
        //dup2(error, 2);
        //close(error);
      }
      command_clear(command);
      prompt();
      return;
    }

    if (!strcmp(command->simple_commands[i]->arguments[0], "setenv")) {
      char *variable = command->simple_commands[i]->arguments[1];
      char *value = command->simple_commands[i]->arguments[2];
      if (getenv(variable) != NULL) {
        setenv(variable, value, 1);
      }
      else {
        setenv(variable, value, 0);
      }
      command_clear(command);
      prompt();
      return;
    }
    if (!strcmp(command->simple_commands[i]->arguments[0], "unsetenv")) {
      char *variable = command->simple_commands[i]->arguments[1];
      if (!unsetenv(variable)) {
        perror("unsetenv");
      }
      command_clear(command);
      prompt();
      return;
    }

    // fork and excute
    ret = fork();
    if (ret < 0) {
      // fork error
      perror("fork");
      command->last_return = 1;
      return;
    }
    else if (ret == 0) {
      // this is child process, excute
      char *arg_name = command->simple_commands[i]->arguments[0];
      if (!strcmp(arg_name, "printenv")) {
        char **p = __environ;
        while (*p != NULL) {
          printf("%s\n", *p++);
        }
        exit(0);
      }
      else {
        execvp((command->simple_commands[i]->arguments[0]), 
          command->simple_commands[i]->arguments);
        perror("execvp");
        exit(1);
      }
    }
    else {
      //waitpid(ret, NULL, 0);
    }
    // continue
  }

  // restore in/out defaults
  dup2(tmpin, 0);
  dup2(tmpout, 1);
  dup2(tmperr, 2);
  close(tmpin);
  close(tmpout);
  close(tmperr);

  // If there is a background process, wait
  if (!command->is_background) {
    command->last_pid = ret;
    waitpid(ret, NULL, 0);
  }

  // Clear to prepare for next command

  command_clear(command);

  // Print next prompt
  prompt();
} /* command_execute() */


/*
 * Print the shell prompt
 */

void prompt() {
  if (isatty(0)) {
    printf("myshell>");
    fflush(stdout);
  }
} /* prompt() */

/*
 * Set up ctrl-c signal
 */

void ctrl_c(int signal) {
  fprintf(stderr, "\n");
  //fflush(stdout);
  if (current_command->is_cleared != 0) {
    command_clear(current_command);
    prompt();
  }
} /* ctrl_c */

/*
 * Set up kill zombie
 */

void zombie(int signal) {
  int status;
  pid_t pid = wait3(0, 0, NULL);
  while (pid > 0) {
    pid = wait3(&status, WNOHANG, (struct rusage *)NULL);
    if (pid == 0 || pid == -1) {
      //prompt();
      return;
    }
  }
  //prompt();
} /* zombie */



/*
 * Start the shell
 */

int main() {
  // initialize the current_command

  current_command = command_create();

  //prompt();

  // run the parser
  struct sigaction sa_ctrl;
  sa_ctrl.sa_handler = ctrl_c;
  sigemptyset(&sa_ctrl.sa_mask);
  sa_ctrl.sa_flags = SA_RESTART;
  if (sigaction(SIGINT, &sa_ctrl, NULL)) {
    perror("sigaction");
    exit(2);
  }

  struct sigaction sa_zombie;
  sa_zombie.sa_handler = zombie;
  sigemptyset(&sa_zombie.sa_mask);
  sa_zombie.sa_flags = SA_RESTART;
  if (sigaction(SIGCHLD, &sa_zombie, NULL)) {
    perror("sigaction");
    exit(2);
  }

  prompt();

  yyparse();

  return 0;
} /* main() */
