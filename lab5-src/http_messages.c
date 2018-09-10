#include "http_messages.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/*
 * Return the reason string for a particular status code. You might find this
 * helpful when implementing response_string().
 */

const char *status_reason(int status) {
  switch (status) {
  case 100:
    return "Continue";
  case 101:
    return "Switching Protocols";
  case 200:
    return "OK";
  case 201:
    return "Created";
  case 202:
    return "Accepted";
  case 203:
    return "Non-Authoritative Information";
  case 204:
    return "No Content";
  case 205:
    return "Reset Content";
  case 206:
    return "Partial Content";
  case 300:
    return "Multiple Choices";
  case 301:
    return "Moved Permanently";
  case 302:
    return "Found";
  case 303:
    return "See Other";
  case 304:
    return "Not Modified";
  case 305:
    return "Use Proxy";
  case 307:
    return  "Temporary Redirect";
  case 400:
    return "Bad Request";
  case 401:
    return "Unauthorized";
  case 402:
    return "Payment Required";
  case 403:
    return "Forbidden";
  case 404:
    return "Not Found";
  case 405:
    return "Method Not Allowed";
  case 406:
    return "Not Acceptable";
  case 407:
    return "Proxy Authentication Required";
  case 408:
    return "Request Time-out";
  case 409:
    return "Conflict";
  case 410:
    return "Gone";
  case 411:
    return "Length Required";
  case 412:
    return "Precondition Failed";
  case 413:
    return "Request Entity Too Large";
  case 414:
    return "Request-URI Too Large";
  case 415:
    return  "Unsupported Media Type";
  case 416:
    return "Requested range not satisfiable";
  case 417:
    return "Expectation Failed";
  case 500:
    return "Internal Server Error";
  case 501:
    return "Not Implemented";
  case 502:
    return "Bad Gateway";
  case 503:
    return "Service Unavailable";
  case 504:
    return "Gateway Time-out";
  case 505:
    return "HTTP Version not supported";
  default:
    return "Unknown status";
  }
}

/*
 * Create the actual response string to be sent over the socket, based
 * on the parameter.
 */

char *response_string(http_response *response) {
  // TODO: Replace this code and correctly create the HTTP response from the
  // argument

  char *to_string = strdup(response->http_version);

  int new_length = strlen(to_string) + sizeof(response->status_code) + 2;
  char *status_str = (char *)malloc(sizeof(char) 
    * sizeof(response->status_code));
  sprintf(status_str, "%d", response->status_code);
  to_string = realloc(to_string, new_length);
  strcat(to_string, " ");
  strcat(to_string, status_str);
  strcat(to_string, " ");
  free(status_str);
  
  const int status = response->status_code;
  char *reason = strdup(status_reason(status));
  response->reason_phrase = reason;
  new_length = strlen(to_string) + strlen(reason) + 3;
  to_string = realloc(to_string, new_length);
  strcat(to_string, reason);
  strcat(to_string, "\r\n");
  free(reason);

  int i;
  //printf("headers: %d\n", response->num_headers);
  int content_length = 0;
  for (i = 0; i < response->num_headers; i++) {
    if ((response->headers[i].key != NULL) && 
    (response->headers[i].value != NULL)) {
      new_length = strlen(to_string) + strlen(response->headers[i].key) + 
        strlen(response->headers[i].value) + 5;
      to_string = realloc(to_string, new_length);
      strcat(to_string, response->headers[i].key);
      strcat(to_string, ": ");
      strcat(to_string, response->headers[i].value);
      strcat(to_string, "\r\n");
      char *key = response->headers[i].key;
      if (!strcmp(key, "Content-Length")) {
        content_length = atoi(response->headers[i].value);
        printf("content-length: %d\n", content_length);
      }
    }
  }

  if (content_length == 0) {
    new_length = strlen(to_string) + 5 + strlen(response->message_body);
  }
  else {
    new_length = strlen(to_string) + 5 + content_length;
  }
  to_string = realloc(to_string, new_length);
  strcat(to_string, "\r\n");

  int my_length = strlen(to_string);
  for (i = 0; i < content_length; i++) {
    int index = my_length + i;
    to_string[index] = response->message_body[i];
  }

  int cur_index = my_length + i;
  to_string[cur_index++] = '\r';
  to_string[cur_index++] = '\n';
  to_string[cur_index] = '\0';
  //printf("%s\n", to_string);
  return to_string;
}

/*
 * Print the request to stdout, useful for debugging.
 */

void print_request(http_request *request) {
  // Magic string to help with autograder

  printf("\\\\==////REQ\\\\\\\\==////\n");

  printf("Method: {%s}\n", request->method);
  printf("Request URI: {%s}\n", request->request_uri);
  printf("Query string: {%s}\n", request->query);
  printf("HTTP Version: {%s}\n", request->http_version);

  printf("Headers: \n");
  for (int i = 0; i < request->num_headers; i++) {
    printf("field-name: %s; field-value: %s\n",
           request->headers[i].key, request->headers[i].value);
  }

  printf("Message body length: %ld\n", strlen(request->message_body));
  printf("%s\n", request->message_body);

  // Magic string to help with autograder

  printf("//==\\\\\\\\REQ////==\\\\\n");
}
