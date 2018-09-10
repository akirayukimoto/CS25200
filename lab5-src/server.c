#include "server.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include "http_messages.h"
#include "misc.h"
#include "handle_dir.h"
/*
 * Accept connections one at a time and handle them.
 */

void run_linear_server(acceptor *acceptor) {
  while (1) {
    socket_t *sock = accept_connection(acceptor);
    handle(sock);
  }
}

/*
 * Accept connections, creating a different child process to handle each one.
 */

void run_forking_server(acceptor *acceptor) {
  // TODO: Add your code to accept and handle connections in child processes
  while (1) {
    socket_t *sock = accept_connection(acceptor);
    int ret = fork();
    if (ret == 0) {
      handle(sock);
      exit(0);
    }
    else if (ret > 0) {
      if (waitpid(ret, NULL, 0) < 0) {
        perror("child");
        exit(-1);
      }
    }
  }
}

/*
 * Accept connections, creating a new thread to handle each one.
 */

void run_threaded_server(acceptor *acceptor) {
  // TODO: Add your code to accept and handle connections in new threads
}

/*
 * Accept connections, drawing from a thread pool with num_threads to handle the
 * connections.
 */

void run_thread_pool_server(acceptor *acceptor, int num_threads) {
  // TODO: Add your code to accept and handle connections in threads from a
  // thread pool
}

/*
 * Help function: check whether the given path is a directory
 */

int is_directory(const char *path) {
  struct stat path_stat;
  stat(path, &path_stat);
  return S_ISDIR(path_stat.st_mode);
} // is_directory

/*
 * Parsing Request to Respond
 */

void parse_respond(socket_t *sock, http_request * const request) {
  //printf("pt 1\n");
  http_response response;
  char *revised_path = strdup("./http-root-dir");
//  printf("print: %s\n", request->request_uri);
  char *ptr = strdup(request->request_uri);
  //printf("%s\n", ptr);
  if (!strncmp(ptr, "/icons", strlen("/icons")) ||
  !strncmp(ptr, "/htdocs", strlen("/htdocs"))) {
    int new_length = strlen(revised_path) + strlen(ptr) + 1;
    revised_path = realloc(revised_path, new_length);
    strcat(revised_path, ptr);
  }
  else {
    if (ptr[0] == '/') {
      //printf("what about here\n");
      int new_length = strlen(revised_path) + strlen("/htdocs") + 
        strlen(ptr) + 2;
      revised_path = realloc(revised_path, sizeof(char) * new_length);
      strcat(revised_path, "/htdocs");
      //char *ptr = strdup(request->request_uri);
      //printf("what about here\n");
      strcat(revised_path, ptr);
      //printf("what a:qbout here\n");
      //printf("%s\n", revised_path);
    }
    else {
      int new_length = strlen(revised_path) + strlen("/htdocs/") 
        + strlen(ptr) + 1;
      revised_path = realloc(revised_path, sizeof(char) * new_length);
      strcat(revised_path, "/htdocs/");
      strcat(revised_path, request->request_uri);
    }
  }
  free(ptr);
  printf("revised path: %s\n", revised_path);
  int status;
  char *content_type;
//  printf("content_type: %s\n", content_type);
  //int max_chars = 1024;
  int is_dir = 0;
  char *message_body;
  FILE *f;
  f = fopen(revised_path, "r+");
  int message_length = 0;
  if (f == NULL) {
    if (!is_directory(revised_path)) {
      printf("here\n");
      status = 404;
      content_type = strdup("text/plain");
      message_body = strdup("404 Not Found");
    }
    else {
      is_dir = 1;
      status = 200;
    }
  }
  else {
//    printf("here?\n");
    status = 200;
    content_type = get_content_type(revised_path);
    printf("content_type: %s\n", content_type);

    int num = 0;
    int is_image = 0;
    if (!strncmp(content_type, "image", strlen("image"))) {
      fclose(f);
      f = fopen(revised_path, "rb");
      is_image = 1;
    }
    if (!is_image) {
      fseek(f, 0, SEEK_END);
      num = ftell(f);
      fseek(f, 0, SEEK_SET);
      message_body = (char *)malloc(sizeof(char) * num);
      //printf("whattttttt!\n");
      //message_body[num] = ch;
      //num++;
      if (message_body != NULL) {
        fread(message_body, 1, num, f);
      }
      //printf("num: %d\n", num);
      message_length = num;
    }
    else {
      char ch;
      int max_char = 10240;
      message_body = (char *)malloc(sizeof(char) * max_char);
      //char *p = message_body;
      while(read(fileno(f), &ch, sizeof(ch)) != 0) {
        message_body[num] = ch;
        num++;
        if (num == max_char) {
          max_char = max_char * 2;
          message_body = realloc(message_body, sizeof(char) * max_char);
        }
      }
      message_body[num] = 0;
      message_length = num;
    }
    free(revised_path);
    fclose(f);
  }
  if (is_dir == 1) {
    request->request_uri = revised_path;
    handle_directory(sock, request);
    free(revised_path);
//    char *to_string = response_string(&response);
//    socket_write_string(sock, to_string);
//    free(to_string);
  }
  else if (status == 200) {
    //printf("hello\n");
    header *header_array = (header *)malloc(sizeof(header) * 3);

    response.http_version = request->http_version;
    response.status_code = status;

    header content_type_head;
    content_type_head.key = "Content-Type";
    content_type_head.value = content_type;
    header connection_head;
    connection_head.key = "Connection";
    connection_head.value = "close";

    header message_length_head;
    message_length_head.key = "Content-Length";
    char *msg_length = (char *)malloc(sizeof(char) * message_length);
    //int tmp = strlen(message_body);
    sprintf(msg_length, "%d", message_length);
    message_length_head.value = msg_length;
    printf("msg len: %s\n", msg_length);

    header_array[0] = content_type_head;
    header_array[1] = connection_head;
    header_array[2] = message_length_head;
    printf("what\n");
    response.headers = header_array;
    response.num_headers = 3;

    //response.message_body = (char *)malloc(sizeof(char) * message_length);
    response.message_body = message_body;
    //printf("what2\n");
    char *to_string = response_string(&response);
    printf("%s", to_string);
    size_t my_length = strlen(to_string) - strlen(message_body) 
      + sizeof(char) * message_length + sizeof(char);
    socket_write(sock, to_string, sizeof(char) * my_length);
    free(header_array);
    free(message_body);
    free(content_type);
    free(msg_length);
    free(to_string);
  }
  else if (status == 404) {
    response.http_version = "HTTP/1.1";
    header *header_array = (header *)malloc(sizeof(header) * 3);
    response.status_code = status;
    header content_type_head;
    content_type_head.key = "Content-Type";
//    printf("contenttype: %s\n", content_type);
    content_type_head.value = content_type;
    header connection_head;
    connection_head.key = "Connection";
//    printf("whatttttttttttt\n");
    connection_head.value = "close";

    header message_length_head;
    int tmp = strlen(message_body);
    char *message_length = (char *)malloc(sizeof(char) * tmp);
    sprintf(message_length, "%d", tmp);
    message_length_head.key = "Content-Length";
    message_length_head.value = message_length;
//    printf("whatttttttttttt\n");

    header_array[0] = content_type_head;
    header_array[1] = connection_head;
    header_array[2] = message_length_head;
//    printf("whatttttttttttt\n");

    response.headers = header_array;
    response.num_headers = 3;

//    printf("whatttttttttttt\n");
    response.message_body = message_body;
    //fclose(f);
// printf("whatttttttttttt\n");
    char *to_string = response_string(&response);
//     printf("whatttttttttttt\n");
//    printf("to_string: %s\n", to_string);
    //int my_length = strlen(to_string) - strlen(message_body) + message
    socket_write_string(sock, to_string);
    free(header_array);
    free(message_body);
    free(content_type);
    free(message_length);
    free(to_string);
  }
  //free(to_string);
}

/*
 * Handle an incoming connection on the passed socket.
 */

void handle(socket_t *sock) {
  http_request request;

  // TODO: Replace this code and actually parse the HTTP request
  /*
  request.method = "GET";
  request.request_uri = "/";
  request.query = "";
  request.http_version = "HTTP/1.1";
  request.num_headers = 0;
  request.message_body = "";

  print_request(&request);
  */
  int max_lines = 1;
  int max_headers = 1;
  int num_lines = 0;
  //printf("hello\n");
  char *buffer = socket_read_line(sock);
  //printf("hello\n");
  char **general_lines = (char **)malloc(sizeof(char *) * max_lines);
  //printf("heyhey:\n%s\n", buffer);
  // Read all lines of request and save them in a string array
  
  while (strlen(buffer) > 2) {
    if (num_lines == max_lines) {
      max_lines = max_lines + 1;
      general_lines = realloc(general_lines, sizeof(char *) * max_lines);
    }
    //char *ptr = strtok(buffer, "\r");
    buffer[strlen(buffer) - 2] = '\0';
    printf("buffer: %s\n", buffer);
    general_lines[num_lines] = strdup(buffer);
    printf("general lines: \n%s\n", general_lines[num_lines]);
    free(buffer);
    buffer = socket_read_line(sock);
    num_lines++;
  }
  printf("here!!!\n");
  char *method;
  char *raw_path;
  char *http_version;

  // save info of method, uri, and http_version
  if (general_lines[0] == NULL) {
    perror("socket");
    exit(-1);
  }
  char *first_line = strdup(general_lines[0]);

  char *p = strtok(first_line, " ");
  int count = 0;
  while (p != NULL) {
    if (count == 0)
      method = strdup(p);
    else if (count == 1)
      raw_path = strdup(p);
    else if (count == 2)
      http_version = strdup(p);
    count++;
    p = strtok(NULL, " ");
  }

  free(first_line);
  first_line = NULL;
  if (!strcmp(raw_path, "/")) {
    raw_path = realloc(raw_path, sizeof(char) * 
      (strlen(raw_path) + strlen("index.html") + 1));
    strcat(raw_path, "index.html");
  }

  request.method = method;
  request.http_version = http_version;
  request.request_uri = raw_path;
  request.query = "";
  free(buffer);
  buffer = NULL;

  // add headers
  header *header_array = (header *)malloc(sizeof(header) * max_headers);
  int num_headers = 0;
  int i;
  for (i = 0; i < num_lines; i++) {
    if (i != 0) {
      char *line = strdup(general_lines[i]);
      // do something
      //header hd;
      char *key;
      char *value;
      char *ptr = strtok(line, " ");
      int j = 0;
      while (ptr != NULL) {
        if (j == 0) {
          key = strdup(ptr);
        }
        else {
          if (strcmp(ptr, "Basic") != 0) {
            value = strdup(ptr);
            break;
          }
        }
        ptr = strtok(NULL, " ");
        j++;
      }
      free(line);
      line = NULL;
      if (key != NULL && value != NULL) {
        header head;
        key[strlen(key) - 1] = 0;
        head.key = strdup(key);
        head.value = strdup(value);
        free(key);
        free(value);
//        printf("my key: %s\nmy value: %s\n", head.key, head.value);
        if (num_headers == max_headers) {
          max_headers = max_headers + 1;
          header_array = realloc(header_array, sizeof(header) * max_headers);
        }
        header_array[num_headers] = head;
        num_headers++;
        //free(key);
        //free(value);
      }
    }
  }

  printf("num headers: %d\n", num_headers);
  request.headers = header_array;
  request.num_headers = num_headers;
  request.message_body = "";

  print_request(&request);

  //http_response response;

  // TODO: Add your code to create the correct HTTP response
  //printf("Am I here?\n");
//  free(header_array);
//  free(method);
//  free(raw_path);
//  free(http_version);
  parse_respond(sock, &request);
  int j;
  for (j = 0; j < num_lines; j++) {
    free(general_lines[j]);
  }
  free(general_lines);
  general_lines = NULL;
  for (j = 0; j < num_headers; j++) {
    free(header_array[j].key);
    free(header_array[j].value);
  }
  free(header_array);
  free(method);
  free(raw_path);
  free(http_version);
/*
  for (j = 0; j < num_headers; j++) {
    free(header_array[j].key);
    free(header_array[j].value);
  }
*/
//  free(header_array);
//  header_array = NULL;
/*
  response.http_version = "HTTP/1.1";

  char *to_string = response_string(&response);
  printf("%s\n", to_string);
  socket_write_string(sock, to_string);

  free(to_string);
  to_string = NULL;
*/
  //free(buffer);
  close_socket(sock);
}
