#include "tls.h"

/*
 * Close and free a TLS socket created by accept_tls_connection(). Return 0 on
 * success. You should use the polymorphic version of this function, which is
 * close_socket() in socket.c.
 */

int close_tls_socket(tls_socket *socket) {
  // TODO: Add your code to close the socket

  free(socket);

  return -1;
}

/*
 * Get a character from the TLS socket. You should use the polymorphic version
 * of this function, which is socket_getc() in socket.c.
 */

char tls_getc(tls_socket *socket) {
  // TODO: Add your code to get a character from the socket

  char c = '\0';

  return c;
}

/*
 * Write a buffer of length buf_len to the TLS socket. Return 0 on success. You
 * should use the polymorphic version of this function, which is socket_write()
 * in socket.c
 */

int tls_write(tls_socket *socket, char *buf, size_t buf_len) {
  if (buf == NULL) {
    return -1;
  }

  // TODO: Add your code to write to the socket

  return 0;
}

/*
 * Create a new TLS socket acceptor, listening on the given port. Return NULL on
 * error. You should ues the polymorphic version of this function, which is
 * create_socket_acceptor() in socket.c.
 */

tls_acceptor *create_tls_acceptor(int port) {
  tls_acceptor *acceptor = malloc(sizeof(tls_acceptor));

  // TODO: Add your code to create and listen on the socket acceptor

  return acceptor;
}

/*
 * Accept a new connection from the TLS socket acceptor. Return NULL on error,
 * and the new TLS socket otherwise. You should use the polymorphic version of
 * this function, which is accept_connection() in socket.c.
 */

tls_socket *accept_tls_connection(tls_acceptor *acceptor) {
  // TODO: Add your code to create the new socket

  return NULL;
}

/*
 * Close and free the passed TLS socket acceptor. Return 0 on success. You
 * should use the polymorphic version of this function, which is
 * close_socket_acceptor() in socket.c.
 */

int close_tls_acceptor(tls_acceptor *acceptor) {
  // TODO: Add your code to close the master socket

  free(acceptor);
  return -1;
}
