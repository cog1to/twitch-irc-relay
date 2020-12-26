#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "socket.h"
#include "irc.h"

/** Private **/

/**
 * Wrapper for message retrieval.
 * Checks for NULL and exits the process in case of an error.
 *
 * @param irc: IRC client.
 *
 * @return: Pointer to the new message data.
 **/
irc_message_t *get_next_message(irc_t *irc);

/**
 * Attempts to connect to a server and join a channel.
 *
 * @param server: Host name.
 * @param port: Port number.
 * @param user: Username to identify self.
 * @param password: Password string.
 * @param channel: Channel to join.
 *
 * @return IRC client or NULL.
 **/
irc_t *do_connect(char *server, int port, char *user, char *password, char *channel);

/**
 * Prints the message to stdout.
 *
 * @param message: Message to print.
 **/
void outputMessage(irc_message_t *message);

/**
 * Prints out usage info to STDERR.
 **/
void printUsage();

/**
 * Reads a line from given file descriptor.
 *
 * @param buffer: Buffer to write into.
 * @param size: Max amount of bytes to read.
 * @param fd: File descriptor to read from.
 *
 * @return: Number of bytes read.
 **/
int readLine(char *buffer, int size, int fd);

/** Constants **/

/* Input data path.  */
char const * const FIFO_PATH = "/tmp/irc_client";
/* Input message buffer size. */
int const INPUT_BUFFER_SIZE = 1024;

/** Main **/

int main(int argc, char **argv) {
  char *server = "irc.chat.twitch.tv";
  int port = 6667;

  char *user, *password, *channel;
  if (argc < 4) {
    printUsage();
    exit(0);
  }

  user = argv[1];
  password = argv[2];
  channel = argv[3];

  printf("DEBUG :Connecting to IRC\n");

  // Connect.
  irc_t *irc = do_connect(server, port, user, password, channel);
  if (irc == NULL) {
    exit(-1);
  }

  // Message buffer.
  unsigned int connected = 1;
  irc_message_t *message = NULL;

  printf("DEBUG :Opening a FIFO\n");

  // Input.
  int error = mkfifo(FIFO_PATH, S_IRUSR | S_IWUSR);
  if (error != 0) {
    perror("Failed to create a FIFO");
    exit(-1);
  }
  int input_fd = open(FIFO_PATH, O_RDWR);
  char command[INPUT_BUFFER_SIZE];

  // We want to wait for either FIFO input or socket data.
  fd_set readfds;

  printf("DEBUG :Entering the main loop\n");

  // Enter a loop TODO: [until SIGKILL]
  while (connected) {
    FD_ZERO(&readfds);
    FD_SET(irc_get_fd(irc), &readfds);
    FD_SET(input_fd, &readfds);

    int maxfd = irc_get_fd(irc);
    if (maxfd < input_fd) {
      maxfd = input_fd;
    }

    int activity = select(maxfd + 1, &readfds, NULL, NULL, NULL);
    if (FD_ISSET(input_fd, &readfds)) {
      memset(command, 0, INPUT_BUFFER_SIZE);
      if ((readLine(command, sizeof(command), input_fd)) != -1) {
        irc_command(irc, "%s\n", command);
      }
    }

    if (FD_ISSET(irc_get_fd(irc), &readfds)) {
      do {
        message = irc_next_message(irc);
        if (message == NULL) {
          if (irc_is_connected(irc) == 0) {
            irc_free(irc);
            irc_t *irc = do_connect(server, port, user, password, channel);
            if (irc == NULL) {
              break;
            }
          }
        } else {
          // Parse the message
          // if it's PRIVMSG to channel, send it to STDOUT
          // if it's anything else, ignore
          if (strcmp(message->command, "PRIVMSG") == 0) {
            outputMessage(message);
          } else if (strcmp(message->command, "PING") == 0) {
            irc_command(irc, "PONG %s", user);
          } else {
            fprintf(stdout, "%s: %s - %s\n", message->sender, message->command, message->message);
            // Do nothing for now.
          }

          // Free message memory.
          irc_message_free(message);
        }
      } while (message != NULL);
    }
  }

  // Clean up.
  if (irc != NULL) {
    irc_free(irc);
  }

  // Close the streams.
  fprintf(stdout, "%d", EOF);
  close(input_fd);

  // Exit without errors.
  return 0;
}

irc_message_t *get_next_message(irc_t *irc) {
  irc_message_t *message = irc_next_message(irc);
  return message;
}

irc_message_t *wait_for_next_message(irc_t *irc) {
  irc_message_t *message = irc_wait_for_next_message(irc);
  if (message == NULL) {
    perror("Failed to wait for the next message.");
    exit(-1);
  }
  return message;
}

irc_t *do_connect(char *server, int port, char *user, char *password, char *channel) {
  // Connect to socket.
  int socket_fd = sock_connect(server, port);
  if (socket_fd == -1) {
    perror("Failed to connect to server");
    return NULL;
  }

  // Initialize IRC.
  irc_t *irc = irc_init(socket_fd);
  if (irc == NULL) {
    perror("Failed to create IRC client");
    return NULL;
  }

  // Command buffer.
  irc_message_t *message = NULL;

  // Send NICK and PASS, wait for MOTDEND message.
  irc_command(irc, "PASS %s", password);
  irc_command(irc, "NICK %s", user);

  for (int found = 0; found < 1; ) {
    message = wait_for_next_message(irc);
    if (strcmp(message->command, "376") == 0) {
      found = 1;
    }
    irc_message_free(message);
  }

  irc_command(irc, "CAP REQ :twitch.tv/tags twitch.tv/commands");
  for (int found = 0; found < 1; ) {
    message = wait_for_next_message(irc);
    if (strcmp(message->command, "CAP") == 0) {
      found = 1;
    }
    irc_message_free(message);
  }

  // Send JOIN message, wait for NICK list end response.
  irc_command(irc, "JOIN #%s", channel);
  for (int joined = 0; joined < 1; ) {
    message = wait_for_next_message(irc);
    if (strcmp(message->command, "366") == 0) {
      joined = 1;
    }
    irc_message_free(message);
  }

  return irc;
}

void outputMessage(irc_message_t *message) {
  if (message->sender == NULL || message->tags == NULL || message->message == NULL) {
    return;
  }

  fprintf(stdout, "%s\t%s", message->tags, message->sender);
  if (message->command != NULL)
    fprintf(stdout, "\t%s", message->command);
  fprintf(stdout, "\t%s\n", message->message);
}

void printUsage() {
  fprintf(stderr,
      "Usage: client <user> <password> <channel>\n"
  );
}

int readLine(char *buffer, int size, int fd) {
  char c;
  int counter = 0;
  while ((read(fd, &c, 1) != 0) && counter < size) {
    if (c == '\n') {
      break;
    }
    buffer[counter++] = c;
  }
  return counter;
}

