/*
 *
 * CS 252 Summer 2018
 * shell.l: lexical analyzer for shell
 * You have to extend it.
 *
 */

%{

#include <string.h>

#include "y.tab.h"

#define MAX_BUFFER 1024
/*
char * read_line();

int mygetc(FILE *f) {
  static char *p;
  char ch;

  if (!isatty(0)) {
    return getc(f);
  }

  if (p == NULL || *p = 0) {
    char *s = read_line();
    p = s;
  }

  ch = *p;
  p++;

  return ch;
}
*/
void delete_slashes();

%}

%%

\n {
  return NEWLINE;
}

[ \t]+ {
  // Discard spaces and tabs

}

">" {
  return GREAT;
}

">>" {
 return GREATGREAT;
}

"&" {
  return AMPERSAND;
}

">&" {
  return GREATAMP;
}

"&<" {
  return GREATAMP;
}
 
">>&" {
  return GREATGREATAMP;
}

"&>>" {
  return GREATGREATAMP;
}
 
"<" {
  return LESS;
}

"|" {
  return PIPE;
}

"2>&1" {
  return REDIR;
}

"2>" {
  return REDIR;
}

\"[^\t\n\"]*\" {
  char *tmp = yytext;
  tmp++;
  strcpy(yytext, tmp);
  yytext[strlen(yytext) - 1] = '\0';
  yylval.string_val = strdup(yytext);
  return WORD;
}

[^ \t\n|><&][^ \t\n|><&]* {
  yylval.string_val = strdup(yytext);
  return WORD;
}

[^ \t\n]*(\\[^\s])[^ \t\n]* {
  // delete slashes
  delete_slashes();
  yylval.string_val = strdup(yytext);
  return WORD;
}

`[^\n`]*` {
//  printf("subshell\n");
  char *cmd = strdup(++yytext);
  cmd[strlen(cmd) - 1] = 0;
  strcat(cmd, "\nexit\n");

  int tmpin = dup(0);
  int tmpout = dup(1);

//  printf("current command %s\n", cmd);

  int pipein[2];
  int pipeout[2];
  pipe(pipein);
  pipe(pipeout);

  write(pipein[1], cmd, strlen(cmd));
//  write(pipein[1], "\nexit\n", strlen("\nexit\n"));
  close(pipein[1]);

  dup2(pipein[0], 0);
  dup2(pipeout[1], 1);
  close(pipein[0]);
  close(pipeout[1]);

//  printf("current command:%s\n", cmd);

  int ret = fork();

  if (ret == 0) {  
    char *arg[] = {"/proc/self/exe", ""};
    execvp("/proc/self/exe", arg);
    //printf("Here!\n");
    _exit(1);
  }
  else if (ret < 0) {
    perror("fork");
    exit(1);
  }

  dup2(tmpin, 0);
  dup2(tmpout, 1);
  close(tmpin);
  close(tmpout);

  //waitpid(ret, NULL, 0);
  char ch;
  char buffer[MAX_BUFFER];
  int i = 0;
  while (read(pipeout[0], &ch, 1)) {
    if (ch == '\n') {
      buffer[i] = ' ';
    }
    else {
      buffer[i] = ch;
    }
    i++;
  }
  buffer[i] = 0;

  char *temp = buffer;
  char good_bye[] = " Good bye!! ";

  while (temp = strstr(temp, good_bye)) {
    memmove(temp, temp + strlen(good_bye), 1 + strlen(temp + strlen(good_bye)));
  }

  buffer[strlen(buffer) - 1] = 0;

//  fprintf(stdout, buffer, strlen(buffer));

  for (i = strlen(buffer); i >= 0; i--) {
//    myunputc(buffer[i]);
    unput(buffer[i]);
//    fprintf(stderr, "%c", buffer[i]);
  }
}


%%
void delete_slashes() {
  char temp[strlen(yytext) + 1];
  int i = 0;
  int j = 0;
  while (i < strlen(yytext)) {
    if (yytext[i] != '\\' && yytext[i] != '\n') {
      temp[j] = yytext[i];
      j++;
    }
    else {
      temp[j] = yytext[++i];
      j++;
    }
    i++;
  }
  temp[j] = 0;
  memcpy(yytext, temp, sizeof(temp));
}
