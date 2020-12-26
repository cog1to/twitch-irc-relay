#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

int sock_connect(char *host, int port) {
  int return_value = -1, error, fd = -1;
  struct addrinfo hints, *res = NULL, *rp;

  char port_string[16];
  sprintf(port_string, "%d", port);

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;     /* allow IPv4 or IPv6 */
  hints.ai_flags = AI_NUMERICSERV; /* avoid name lookup for port */
  hints.ai_socktype = SOCK_STREAM;

  if ((error = getaddrinfo(host, port_string, &hints, &res))) {
    return -1;
  }

  for (rp = res; rp; rp = rp->ai_next) {
    if ((fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1)
      continue;

    if ((return_value = connect(fd, res->ai_addr, res->ai_addrlen) == -1)) {
      close(fd);
      fd = -1;
      continue;
    }

    break;
  }

  return fd;
}

int sock_receive(int socket, char *data, short size) {
  int return_value = -1;
  return_value = recv(socket, data, size, MSG_DONTWAIT);
  return return_value;
}

int sock_block_receive(int socket, char *data, short size) {
  int return_value = -1;
  struct timeval tv;
  tv.tv_sec = 20;
  tv.tv_usec = 0;

  if (setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) < 0) {
    return -1;
  }

  return_value = recv(socket, data, size, 0);
  return return_value;
}

int sock_send(int socket, char *data, short size) {
  send(socket, data, size, 0);
}
