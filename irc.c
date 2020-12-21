#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

#include "socket.h"
#include "irc.h"

#define BUFFER_SIZE 2048

/* IRC client instance */
struct irc_t {
  int socket_fd;
  int connected;
  char buffer[BUFFER_SIZE];
};

/**
 * Creates a new IRC client instance.
 *
 * @param connection: Socket file descriptor.
 *
 * @return: A new client instance.
 **/
irc_t *irc_init(int connection) {
  struct irc_t *irc = calloc(1, sizeof(irc_t));
  irc->socket_fd = connection;
  irc->connected = 1;
  return irc;
}

/**
 * Checks whether instance is currently connected.
 *
 * @param irc: IRC client.
 *
 * @return: Flag indicating whether client connection is opened.
 **/
int irc_is_connected(irc_t *irc) {
  return irc->connected;
}

/**
 * Sends a new command through the IRC connection.
 *
 * @param irc: IRC client.
 * @param fmt: Command format string.
 * @param args: Command parameters.
 *
 * @return: 0 if sending is successfull, -1 otherwise.
 **/
int irc_command(irc_t *irc, const char *fmt, ...) {
  // Pasted from irc.c
  va_list vl;
  char outb[1024], *outp = outb;

  va_start(vl, fmt);
  int n = vsnprintf(outp, 1022, fmt, vl);
  va_end(vl);
  outp += n;
  *outp++ = '\r';
  *outp++ = '\n';

  int sent = sock_send(irc->socket_fd, outb, n + 2);
  if (sent > 0) {
    return sent;
  } else {
    irc->connected = 0;
    return -1;
  }
}

int irc_command_literal(irc_t *irc, char *str) {
  int sent = sock_send(irc->socket_fd, str, strlen(str));
}

/**
 * Waits for the next message from IRC connection. Block the thread while doing so.
 *
 * @param irc: IRC client.
 *
 * @return: Pointer to a new message, or NULL in case of an error.
 */
irc_message_t *irc_next_message(irc_t *irc) {
  char *pointer, *token;
  char message_str[BUFFER_SIZE] = { 0 };
  int size;

  // Commands are delimited by newline symbol.
  char *cr_index = strchr(irc->buffer, '\n');

  // Wait until we have enough data for a command.
  fd_set readfds;

  while (cr_index == NULL) {
    FD_ZERO(&readfds);
    FD_SET(irc->socket_fd, &readfds);
    int activity = select(irc->socket_fd + 1, &readfds, NULL, NULL, NULL);

    int current_size = strlen(irc->buffer);
    int read = sock_receive(irc->socket_fd, irc->buffer+current_size, BUFFER_SIZE - current_size - 1);
    cr_index = strchr(irc->buffer, '\n');
  }

  // We got a message, let's parse.
  irc_message_t *message = calloc(1, sizeof(struct irc_message_t));

  size = cr_index - irc->buffer;
  strncpy(message_str, irc->buffer, size);
  message_str[size] = '\0';

  token = NULL;
  pointer = message_str;

  // Handle PING
  if (strstr(pointer, "PING") != NULL) {
     token = strsep(&pointer, " ");
     message->command = calloc(strlen(token) + 1, sizeof(char));
     strcpy(message->command, token);

     token = strsep(&pointer, "\0");
     message->sender = calloc(strlen(token) + 1, sizeof(char));
     strcpy(message->sender, token);
  } else {
    // TAGS
    if (message_str[0] == '@') {
      token = strsep(&pointer, " ");
      if (token != NULL) {
        message->tags = calloc(strlen(token) + 1, sizeof(char));
        strcpy(message->tags, token);
      }
    }

    // SENDER
    token = strsep(&pointer, " ");
    if (token != NULL) {
      message->sender = calloc(strlen(token) + 1, sizeof(char));
      strcpy(message->sender, token);
    }

    // COMMAND
    token = strsep(&pointer, " ");
    if (token != NULL) {
      message->command = calloc(strlen(token) + 1, sizeof(char));
      strcpy(message->command, token);
    }

    // RECIPIENT
    token = strsep(&pointer, " ");
    if (token != NULL) {
      message->recipient = calloc(strlen(token), sizeof(char));
      strcpy(message->recipient, token);
      message->recipient[strlen(token)] = '\0';
    }

    // MESSAGE
    token = strsep(&pointer, "\0");
    if (token != NULL) {
      char *trimmed = token;
      if (trimmed[0] == ':') {
        trimmed += 1;
      }
      message->message = calloc(strlen(trimmed) + 1, sizeof(char));
      strcpy(message->message, trimmed);
    }
  }

  // Shift buffer.
  char *ptr = irc->buffer;
  int length = cr_index - ptr + 1;
  int rest = BUFFER_SIZE - length;
  memcpy(ptr, ptr + length, rest);
  memset(ptr + rest, '\0', length);

  return message;
}

/**
 * Disconnects the IRC client and frees the memory occupied by it.
 *
 * @param irc: IRC client to deallocate.
 **/
void irc_free(irc_t *irc) {
  close(irc->socket_fd);
  free(irc);
}

/**
 * Deallocates message structure.
 *
 * @param message: Message to deallocate.
 **/
void irc_message_free(irc_message_t *message) {
  if (message->sender != NULL) { free(message->sender); }
  if (message->command != NULL) { free(message->command); }
  if (message->recipient != NULL) { free(message->recipient); }
  if (message->message != NULL) { free(message->message); }
  if (message->tags != NULL) { free(message->tags); }
  free(message);
}
