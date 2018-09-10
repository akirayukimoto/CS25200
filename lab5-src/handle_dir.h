#ifndef INCLUDE_HANDLE_DIR_HH_
#define INCLUDE_HANDLE_DIR_HH_

#include "http_messages.h"
#include "socket.h"

struct info {
  char *name;
  char *date;
  char *size;
  int is_dir;
} info;

void handle_directory(socket_t *sock, const http_request *request);

#endif
