//
// CS-252
// shell.y: parser for shell
//
// This parser compiles the following grammar:
//
//   cmd [arg]* [> filename]
//
// you must extend it to understand the complete shell grammar
//


// define all tokens that are used in the lexer here:

%token <string_val> WORD
%token NOTOKEN GREAT NEWLINE GREATGREAT AMPERSAND GREATAMP 
GREATGREATAMP LESS NUMBER PIPE REDIR

%union {
  // specify possible types for yylval, for access in shell.l

  char *string_val;

  // int numerical_val;
}

%{
#include <assert.h>

#include <dirent.h>

#include <pwd.h>

#include <regex.h>

#include <stdio.h>

#include <stdlib.h>

#include <string.h>

#include <unistd.h>

#include "command.h"

// yyerror() is defined at the bottom of this file

void yyerror(const char * s);

void expand_wildcards(char *prefix, char *suffix);

void insert_arguments(char *arg);

// We must offer a forward declaration of yylex() since it is
// defined by flex and not available until linking.

int yylex();

%}

%%

goal:
  commands
  ;

commands:
  command
  | commands command
  ;

command:
  simple_command
  ;

simple_command:
  pipe_list iomodifier_list background_opt NEWLINE {
    //printf("   Yacc: Execute command\n");
    command_execute(current_command);
  }
  | NEWLINE {
    command_clear(current_command);
    prompt();
  }
  | error NEWLINE {
    yyerrok;
  }
  ;

command_and_args:
  command_word argument_list {
    command_insert_simple_command(current_command, current_simple_command);
    current_command->is_cleared = 0;
  }
  ;

argument_list:
  argument_list argument
  | // can be empty
  ;

argument:
  WORD {
    //printf("   Yacc: insert argument \"%s\"\n", $1);

    //simple_command_insert_argument(current_simple_command, $1);
    //current_simple_command->is_cleared = 0;
    insert_arguments($1);
  }
  ;

command_word:
  WORD {
    //printf("   Yacc: insert command \"%s\"\n", $1);

    current_simple_command = simple_command_create();
    simple_command_insert_argument(current_simple_command, $1);
  }
  ;

iomodifier_opt:
  GREAT WORD {
    // Rewrite the file
    current_command->out_file = strdup($2);
    current_command->outfile_counter++;
  }
  | GREATGREAT WORD {
      // Append the output to the output file

      current_command->out_file = strdup($2);
      current_command->is_appended = 1;
      current_command->outfile_counter++;
  }
  | GREATAMP WORD {
      // The file is both output file and error file

      //current_command->out_file = strdup($2);
      current_command->err_file = strdup($2);
      //current_command->outfile_counter++;
  }
  | GREATGREATAMP WORD {
      // Append the system error to the file
      current_command->err_file = strdup($2);
      current_command->is_appended = 1;
      //current_command->outfile_counter++;
  }
  | LESS WORD {
      // Input file
      current_command->in_file = strdup($2);
  }
  | REDIR WORD {
      current_command->err_file = strdup($2);
  }
  ;

iomodifier_list:
  iomodifier_list iomodifier_opt
  |
  ;

background_opt:
  AMPERSAND {
    //printf("   Yacc: insert output \"AMPERSAND\"\n");
    current_command->is_background = 1;
  }
  |
  ;

pipe_list:
  pipe_list PIPE command_and_args
  | command_and_args
  ;

%%

/*
 * On parser error, just print the error
 */

void yyerror(const char *message) {
  fprintf(stderr, "%s", message);
} /* yyerror() */

int max_entries = 20;
int num_entries = 0;
char **dir_array = NULL;

int compare_file(const void *f1, const void *f2) {
  const char *ff1 = *(const char **)f1;
  const char *ff2 = *(const char **)f2;
  return strcmp(ff1, ff2);
}

#define MAX_FILENAME 1024

void expand_wildcards(char *prefix, char *suffix) {
  // if the suffix is empty, put prefix in argument
  if (suffix[0] == 0) {
    return;
  }

  char *s = strchr(suffix, '/');
  char component[MAX_FILENAME];
  if (s != NULL) {
    // copy up to the first "/"
    if ((prefix == NULL || prefix[0] == 0) && suffix[0] == '/') {
      suffix = s + 1;
      s = strchr(suffix, '/');

      if (s != NULL) {
        strncpy(component, suffix, s - suffix);
        suffix = s + 1;
      }
      else {
        strcpy(component, suffix);
        suffix = suffix + strlen(suffix);
      }
      prefix = strdup("/");
    }
    else { 
      strncpy(component, suffix, s - suffix);
      suffix = s + 1;
    }
  }
  else {
    strcpy(component, suffix);
    suffix = suffix + strlen(suffix);
  }

  char new_prefix[MAX_FILENAME];
  if (!strchr(component, '*') && !strchr(component, '?')) {
    // component does not have wildcards
    if (prefix[0] == 0) {
      sprintf(new_prefix, "%s", component);
    }
    else if (prefix[0] == '/' && prefix[1] == 0) {
      sprintf(new_prefix, "/%s", component);
    }
    else {
      sprintf(new_prefix, "%s/%s", prefix, component);
    }
    expand_wildcards(new_prefix, suffix);
    return;
  }

  //printf("Still need expand!\n");
  //printf("new prefix here: %s\n", new_prefix);

  // Component still have wildcards
  // Start to expand wildcards
  // 1. Convert wildcard to regular expression
  char *reg = (char *)malloc(2 * strlen(component) + 10);
  char *a = component; // pointer to the original argument
  char *r = reg; // pointer to the regular expression
  // mark ^ at the beginning of the regular argument
  *r = '^'; r++;
  //printf("%s\n", component);
//  if (strcmp(component, ".*") && strcmp(component, "\\.")) {
  while (*a != '\0') {
    // convert '*' to '.*'
    if (*a == '*') { 
      *r = '.'; r++; *r = '*'; r++; 
    }
    // convert '?' to '.'
    else if (*a == '?') { 
      *r = '.'; r++; 
    }
    // convert '.' to '\.'
    else if (*a == '.') { 
      *r = '\\'; r++; *r = '.'; r++; 
    }
    else { 
      *r = *a; r++; 
    }
    a++;
  }
  // mark $ at the end of the regular argument
  *r = '$'; r++; *r = '\0';

  // 2. compile regular expression
  regex_t ret;
  int result = regcomp(&ret, reg, REG_EXTENDED | REG_NOSUB);

  if (result != 0) {
    perror("compile");
    return;
  }

  free(reg);

  //printf("prefix: %s\n", prefix);
  // 3. list directory and add as arguments the entries
  DIR *dir;
  //printf("%s\n", prefix);
  char *dir_to_open = strdup(prefix);
  if (prefix[0] == 0 || prefix == NULL) {
    //printf("HERE!\n");
    dir = opendir(".");
  }
  else {
//    printf("I'm I here?\n");
    dir = opendir(dir_to_open);
  }
//  printf("here! %s\n", prefix);
  free(dir_to_open);
  if (dir == NULL) {
    //perror("opendir");
    return;
  }

  struct dirent *ent;
  if (dir_array == NULL) {
    //printf("%p\n", &dir_array);
    dir_array = (char **)malloc(max_entries * sizeof(char *));
    assert(dir_array != NULL);
  }
  //printf("here\n");
  regmatch_t match;
  //int count = 0;
  while ((ent = readdir(dir)) != NULL) {
    if (!regexec(&ret, ent->d_name, 1, &match, 0)) {
      if (num_entries == max_entries) {
        max_entries *= 2;
        dir_array = realloc(dir_array, max_entries * sizeof(char *));
        assert(dir_array != NULL);
      }
      if (ent->d_name[0] != '.') {
        if (prefix[0] == 0 || prefix == NULL) {
          sprintf(new_prefix, "%s", ent->d_name);
        }
        else if (prefix[0] == '/' && prefix[1] == 0) {
          sprintf(new_prefix, "/%s", ent->d_name);
        }
        else {
          sprintf(new_prefix, "%s/%s", prefix, ent->d_name);
        }
    //    printf("new_prefix: %s\n", new_prefix);
    //    printf("suffix: %s\n", suffix);
        expand_wildcards(new_prefix, suffix);
    //    printf("finish expand!\n");
        if (s == NULL) {
          dir_array[num_entries] = strdup(new_prefix);
          num_entries++;
        }
      }
      else {
        if (component[0] == '.') {
          if (prefix[0] == 0 || prefix == NULL) {
            sprintf(new_prefix, "%s", ent->d_name);
          }
          else if (prefix[0] == '/' && prefix[1] == 0) {
            sprintf(new_prefix, "/%s", ent->d_name);
          }
          else {
            sprintf(new_prefix, "%s/%s", prefix, ent->d_name);
          }
      //    printf("new prefix: %s\n", new_prefix);
      //    printf("suffix: %s\n", suffix);
          expand_wildcards(new_prefix, suffix);
          if (s == NULL) {
            dir_array[num_entries] = strdup(new_prefix);
            num_entries++;
          }
        }
      }
    }
  }
  closedir(dir);
  regfree(&ret);
  return;
}

void tilde_expansion(char *arg) {
  if (!strcmp(arg, "~") || !strcmp(arg, "~/")) {
    char *new_dir = getpwnam(getenv("USER"))->pw_dir;
    //memset(arg, new_dir, strlen(new_dir));
    strcpy(arg, new_dir);
  }
  else {
    char *new_arg = strdup("/homes/");
    char *ptr = arg;
    ptr++;
    strcat(new_arg, ptr);
    strcpy(arg, new_arg);
  }
}

char *environment_expansion(char *arg) {
  char *new_argument;
  char *to_match = "${.*}";
  regex_t re;
  regmatch_t match;
  //printf("%s\n", temp);
  //int result = regcomp(&re, arg, REG_EXTENDED | REG_NOSUB)i;
  char *temp = strdup(arg);
//  snprintf(temp, MAX_FILENAME, "^%s$", arg);
  //printf("%s\n", temp);
  int result = regcomp(&re, to_match, 0);
  if (result != 0) {
    fprintf(stderr, "%s: Bad regular expresion \n", temp);
    exit(-1);
  }

  if (!regexec(&re, temp, 0, &match, 0)) {
    //printf("%s\n", temp);
    int max_length = 1024;
    char env_arg[max_length];
    env_arg[0] = 0;
    int i = 0;
    int j = 0;
    while (temp[i] != 0) {
      if (i == max_length) {
        max_length = max_length * 2;
        env_arg[max_length];
      }
      if (temp[i] == '$') {
        //printf("found $\n");
        char *out;
        if ((strchr((char *)(temp + i), '{')) &&
            (strchr((char *)(temp + i), '}'))) {
          char *head = strchr((char *)(temp + i), '{');
          char *tail = strchr((char *)(temp + i), '}');

          head[tail - head] = 0;
          head++;
          out = strdup(head);
        }
        else {
          char *ptr = strchr((char *)temp, '$');
          //ptr++;
          memmove(ptr, ptr+1, strlen(ptr));
          out = strdup(ptr);
          printf("%s\n", ptr);
        }
        //printf("what out is: %s\n", out);
        char *final;
        if (!strcmp(out, "SHELL") || !strcmp(out, "SH")) {
          final = getcwd(NULL, 0);
          if (getcwd(final, max_length) == NULL) {
            perror("cwd");
            exit(-1);
          }
          strcat(final, "/shell");
          //printf("%s\n", final);
          //char actual[MAX_FILENAME];
//          char *shell = "../shell";
//          final = realpath(shell, NULL);
//          if (final == NULL) {
//            perror("realpath");
//            exit(-1);
//          }
        }
        else if (!strcmp(out, "$")) {
          int pid = getpid();
          //printf("out pid: %d\n", pid);
          final = (char *)malloc(sizeof(pid));
          sprintf(final, "%d", pid);
          //printf("%s\n", final);
        }
        else if (!strcmp(out, "!")) {
          //printf("%d\n", current_command->last_pid);
          final = (char *)malloc(sizeof(current_command->last_pid));
          sprintf(final, "%d", current_command->last_pid);
        }
        else if (!strcmp(out, "?")) {
          final = (char *)malloc(sizeof(current_command->last_return));
          sprintf(final, "%d", current_command->last_return);
        }
        else if (!strcmp(out, "_")) {
          //printf("here!\n");
          //int index = current_command->simple_commands[0]->num_arguments("");
          //final = strdup(cur_com);
        }
        else {
          final = getenv(out);
        }
        if (env_arg[0] == 0)
          strcpy(env_arg, final);
        else
          strcat(env_arg, final);
        j = j + strlen(final);
        i = i + strlen(out) + 3;
        //printf("%c\n", env_arg[j]);
        //printf("%c\n", env_arg[j - 1]);
        //printf("%c\n", temp[i]);
      }
      else if (temp[i] != '\\') {
        //printf("Here!");
        env_arg[j] = temp[i];
        j++;
        i++;
        env_arg[j] = 0;
      }
      else {
        i++;
      }
    }
    env_arg[j] = 0;
    //printf("%s\n", env_arg);
    new_argument = strdup(env_arg);
  }
  else {
    new_argument = strdup(arg);
  }
  regfree(&re);
  return new_argument;
}

void insert_arguments(char *arg) {
  if (strchr(arg, '*') == NULL && strchr(arg, '?') == NULL) {
    if (*arg == '~') {
      tilde_expansion(arg);
    }
    else if (strchr(arg, '$')) {
      //char *temp = strchr(arg, '$');
      //int index = (int)(temp - arg);
      //if (arg[index - 1] != '\\') {
        char *new_arg = environment_expansion(arg);
      //printf("%s\n", new_arg);
        arg = new_arg;
      //free(new_arg);
      //}
    }
    simple_command_insert_argument(current_simple_command, arg);
    return;
  }

  expand_wildcards("", arg);
  //printf("here!\n");
  if (dir_array != NULL && num_entries > 1) {
    //printf("herehere!!\n");
    //printf("%s\n", dir_array[i]);
    qsort(dir_array, num_entries, sizeof(char *), compare_file);
    //printf("here!here!\n");
    int i;
    for (i = 0; i < num_entries; i++) {
      //printf("%s\n", dir_array[i]);
      simple_command_insert_argument(current_simple_command, dir_array[i]);
    }
    free(dir_array);
    dir_array = NULL;
    num_entries = 0;
  }
  else {
    simple_command_insert_argument(current_simple_command, arg);
  }
  //dir_array = NULL;
  return;
}
