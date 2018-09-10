#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>

#include "handle_dir.h"

/*
 * Help function: is the given path is a directory?
 */

int is_direc(const char *path) {
  struct stat path_stat;
  stat(path, &path_stat);
  return S_ISDIR(path_stat.st_mode);
}

/*
 * Help function: sort files
 */

int compare_name_ascending(const void *a, const void *b) {
  const struct info *aa = (const struct info *)a;
  const struct info *bb = (const struct info *)b;
  char *name_a = aa->name;
  char *name_b = bb->name;
  return (strcmp(name_a, name_b) > 0);
}

/*
 * If the current path is a directory
 * Using this function to parsing request to response and write to socket
 */

void handle_directory(socket_t *sock, const http_request *request) {
  http_response response;
  response.http_version = request->http_version;
  response.status_code = 200;

  int max_info = 20;
  int num_info = 0;

  struct info * info_array = malloc(sizeof(struct info) * max_info);

  char *dir_path = strdup(request->request_uri);
  // I would like to receive all files/directories info from current directory
  DIR *dir = opendir(dir_path);
  if (dir == NULL) {
    //perror("dirent");
    exit(0);
  }
  //char *home_dir = "./http-root-dir/htdocs";
  struct dirent *ent;
  while ((ent = readdir(dir)) != NULL) {
    // create new file struct
    struct info file_info;
    // add file path to file info
    
    char *file_path = strdup(dir_path);
    char tmp[256];
    strcpy(tmp, file_path);
    char *sub;
    sub = strtok(tmp, "/");
    sub = strtok(NULL, "/");
    sub = strtok(NULL, "/");
    sub = strtok(NULL, "/");
    char *name = strdup(sub);

    file_path = realloc(file_path, strlen(file_path) + 2);
    strcat(file_path, "/");
    name = realloc(name, strlen(name) + 2);
    strcat(name, "/");

    printf("ent->d_name: %s\n", ent->d_name);
    int new_len = strlen(file_path) + strlen(ent->d_name) + 1;
    file_path = realloc(file_path, sizeof(char) * new_len);
    strcat(file_path, ent->d_name);
    new_len = strlen(name) + strlen(ent->d_name) + 1;
    name = realloc(name, sizeof(char) * new_len);
    strcat(name, ent->d_name);
    file_info.name = strdup(name);
    free(name);
    //char *check_dir = malloc(sizeof(char) * 
    //  (strlen(home_dir) + strlen(file_path) + 1));
    //strcat(check_dir, home_dir);
    //strcat(check_dir, file_path);

    // add file size info to file info
    if (is_direc(file_path)) {
      // if it is a directory, no need to show its size
      file_info.size = strdup("-");
      file_info.is_dir = 1;
    }
    else {
      struct stat st;
      stat(file_path, &st);
      int size = st.st_size;
      new_len = sizeof(size) + 1;
      char *size_str = malloc(sizeof(char) * new_len);
      sprintf(size_str, "%d", size);
      file_info.size = strdup(size_str);
      free(size_str);
      file_info.is_dir = 0;
    }

    // add file last modified date to file info
    struct tm *foo;
    struct stat attrib;
    stat(file_path, &attrib);
    foo = gmtime(&(attrib.st_mtime));
    int year = foo->tm_year + 1900;
    int month = foo->tm_mon + 1;
    int day = foo->tm_mday;
    int hour = foo->tm_hour;
    int min = foo->tm_min;
    new_len = sizeof(foo->tm_year) + sizeof(foo->tm_mon) + sizeof(foo->tm_mday)
      + sizeof(foo->tm_hour) + sizeof(foo->tm_min) + 6;
    char *md_time = malloc(sizeof(char) * new_len);
    sprintf(md_time, "%d-%d-%d, %d:%d", month, day, year, hour, min);
    file_info.date = strdup(md_time);
    
    // if the array is full, expand the size of the info array
    if (num_info == max_info) {
      max_info = max_info * 2;
      info_array = realloc(info_array, sizeof(struct info) * max_info);
    }
    info_array[num_info] = file_info;
    free(file_path);
    //free(check_dir);
    free(md_time);
    num_info++;
  }
  closedir(dir);

  // sort info array by name
  if (num_info > 1) {
    qsort(info_array, num_info, sizeof(struct info), compare_name_ascending);
  }

  // write to response
  header *header_array = (header *)malloc(sizeof(header) * 3);
  header content_type_header;
  content_type_header.key = "Content-Type";
  content_type_header.value = "text/html";
  header connection_header;
  connection_header.key = "Connection";
  connection_header.value = "close";
  header_array[0] = content_type_header;
  header_array[1] = connection_header;

  // create the directory page and save it in message_body
  char *message_body = strdup("<html><head><title>");
  char *part = "Index of Directory</title></head><body><h1>Index of";
  int new_length = strlen(message_body) + strlen(part) + 1;
  message_body = realloc(message_body, new_length);
  strcat(message_body, part);

  new_length = strlen(message_body) + strlen(dir_path) + 1;
  message_body = realloc(message_body, sizeof(char) * new_length);
  strcat(message_body, dir_path);
  char *tag_hd_end = "</h1>";
  char *tag_title = "<table><th>Name</th><th>Date</th><th>Size</th>";
  new_length = strlen(message_body) + strlen(tag_hd_end) 
    + strlen(tag_title) + 1;
  message_body = realloc(message_body, sizeof(char) * new_length);
  strcat(message_body, tag_hd_end);
  strcat(message_body, tag_title);

  // required tags
  char *tag_tr_hd = "<tr>";
  char *tag_td_hd = "<td>";
  char *tag_a_hd = "<A HREF=\"";
  char *tag_a_bk = "\">";
  char *tag_a_end = "</A>";
  char *tag_td_end = "</td>";
  char *tag_tr_end = "</tr>";

  int i;
  for (i = 0; i < num_info; i++) {
    // add name to html page
//    char tmp_string[256];
//    strcpy(tmp_string, info_array[i].name);
//    char *sub_string;
//    sub_string = strtok(tmp_string, "/");
//    sub_string = strtok(NULL, "/");
//    sub_string = strtok(NULL, "/");
//    sub_string = strtok(NULL, "/");
//    printf("filepath :%s\n", sub_string);
    new_length = strlen(message_body) + strlen(tag_tr_hd) 
      + strlen(tag_td_hd)
      + strlen(tag_a_hd) + 2 * strlen(info_array[i].name) 
      + strlen(tag_a_bk) + strlen(tag_a_end) 
      + strlen(tag_td_end) + 1;
    message_body = realloc(message_body, sizeof(char) * new_length);
    strcat(message_body, tag_tr_hd);
    strcat(message_body, tag_td_hd);
    strcat(message_body, tag_a_hd);
    strcat(message_body, info_array[i].name);
    strcat(message_body, tag_a_bk);
    strcat(message_body, info_array[i].name);
    strcat(message_body, tag_a_end);
    strcat(message_body, tag_td_end);

    // add date to html page
    new_length = strlen(message_body) + strlen(tag_td_hd) 
      + strlen(info_array[i].date) + strlen(tag_td_end) + 1;
    message_body = realloc(message_body, sizeof(char) * new_length);
    strcat(message_body, tag_td_hd);
    strcat(message_body, info_array[i].date);
    strcat(message_body, tag_td_end);

    // add size to html page
    new_length = strlen(message_body) + strlen(tag_td_hd)
      + strlen(info_array[i].size) + strlen(tag_td_end) 
      + strlen(tag_tr_end) + 1;
    message_body = realloc(message_body, sizeof(char) * new_length);
    strcat(message_body, tag_td_hd);
    strcat(message_body, info_array[i].size);
    strcat(message_body, tag_td_end);
    strcat(message_body, tag_tr_end);
  }

  char *rest_tags = "</table></body></html>";
  new_length = strlen(message_body) + strlen(rest_tags) + 1;
  message_body = realloc(message_body, sizeof(char) * new_length);
  strcat(message_body, rest_tags);
  //printf("message:body: %s\n", message_body);
  // add content length to headers
  header content_length_header;
  int str_len = strlen(message_body);
  char *message_length = malloc(sizeof(char) * str_len);
  sprintf(message_length, "%d", str_len);
  printf("dir msg len: %s\n", message_length);
  content_length_header.key = "Content-Length";
  content_length_header.value = message_length;
  header_array[2] = content_length_header;

  response.headers = header_array;
  response.num_headers = 3;
  response.message_body = message_body;
  printf("message_body: %s\n", response.message_body);

  // write to socket
  char *to_string = response_string(&response);
  printf("to_string:\n%s\n", to_string);
  socket_write_string(sock, to_string);
  printf("here\n");
  // free
  free(to_string);
  free(header_array);
  free(dir_path);
  for (i = 0; i < num_info; i++) {
    free(info_array[i].name);
    free(info_array[i].date);
    free(info_array[i].size);
  }
  free(info_array);
  free(message_body);
  free(message_length);
//  return response;
}
