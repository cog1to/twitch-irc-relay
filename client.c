#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#include "socket.h"
#include "irc.h"
#include "commands/list.h"
#include "debug.h"

/** Commands **/

COMMAND(hi)
COMMAND(trnm)

void register_commands() {
  REGISTER(hi)
  REGISTER(trnm)
}

/** Signal handling **/

/* Indicates that program should stop and clean up. */
static volatile int terminate = 0;

/**
 * Process signal handler.
 *
 * @param signal: Received signal.
 **/
static void signal_handler(int signal) {
  terminate = 1;
}

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
 * @param file: File handle to output into.
 * @param message: Message to print.
 **/
void output_message(int file, irc_message_t *message);

/**
 * Prints out usage info to STDERR.
 **/
void print_usage();

/**
 * Reads a line from given file descriptor.
 *
 * @param buffer: Buffer to write into.
 * @param size: Max amount of bytes to read.
 * @param fd: File descriptor to read from.
 *
 * @return: Number of bytes read.
 **/
int read_line(char *buffer, int size, int fd);

/**
 * Sets up signal handling.
 *
 * @param sigset: Signal set to modify.
 */
void setup_signals(sigset_t *sigset);

/** Constants **/

/* Input data path.  */
char const * const IN_FIFO_PATH = "/tmp/twitch-bot-in";
char const * const OUT_FIFO_PATH = "/tmp/twitch-bot-out";
/* Input message buffer size. */
int const INPUT_BUFFER_SIZE = 1024;

/** Main **/

int main(int argc, char **argv) {
  char *server = "irc.chat.twitch.tv";
  int port = 6667;

  char *user, *password, *channel;
  if (argc < 4) {
    print_usage();
    exit(0);
  }

  user = argv[1];
  password = argv[2];
  channel = argv[3];

  // Register commands.
  register_commands();

  LOG(LOG_LEVEL_DEBUG, "DEBUG: Connecting to IRC\n");

  // Connect.
  irc_t *irc = do_connect(server, port, user, password, channel);
  if (irc == NULL) {
    exit(-1);
  }

  // Message buffer.
  irc_message_t *message = NULL;

  LOG(LOG_LEVEL_DEBUG, "DEBUG: Opening a FIFO\n");

  // Input.
  int error = mkfifo(IN_FIFO_PATH, S_IRUSR | S_IWUSR);
  if (error != 0) {
    perror("Failed to create a FIFO");
    exit(-1);
  }

  // Input feed.
  int input_fd = open(IN_FIFO_PATH, O_RDWR);
  char command[INPUT_BUFFER_SIZE];

  // Output.
  error = mkfifo(OUT_FIFO_PATH, S_IRUSR | S_IWUSR);
  if (error != 0) {
    perror("Failed to create a FIFO");
    exit(-1);
  }

  // Output feed.
  int output_fd = open(OUT_FIFO_PATH, O_RDWR);

  // We want to wait for either FIFO input or socket data.
  fd_set readfds;

  // Add signal interruptors.
  sigset_t orig_mask;
  setup_signals(&orig_mask);

  LOG(LOG_LEVEL_DEBUG, "DEBUG: Entering the main loop\n");

  // Main loop (will interrupt on SIGTERM or SIGINT (CTRL+C))
  int reconnect = 0;
  while (terminate == 0) {
    // TODO: Test reconnect. Not sure it works properly...
    if (reconnect) {
      LOG(LOG_LEVEL_DEBUG, "DEBUG: Reconnecting\n");
      irc_free(irc);
      irc_t *irc = do_connect(server, port, user, password, channel);
      if (irc == NULL) {
        break;
      }
      reconnect = 0;
    }

    FD_ZERO(&readfds);
    FD_SET(irc_get_fd(irc), &readfds);
    FD_SET(input_fd, &readfds);

    int maxfd = irc_get_fd(irc);
    if (maxfd < input_fd) {
      maxfd = input_fd;
    }

    int activity = pselect(maxfd + 1, &readfds, NULL, NULL, NULL, &orig_mask);
    if (activity == -1) {
      perror("Error while waiting for the input");
      break;
    }

    if (terminate) {
      LOG(LOG_LEVEL_DEBUG, "DEBUG: Exiting\n");
      break;
    }

    if (FD_ISSET(input_fd, &readfds)) {
      LOG(LOG_LEVEL_DEBUG, "DEBUG: Incoming message\n");
      memset(command, 0, INPUT_BUFFER_SIZE);
      if ((read_line(command, sizeof(command), input_fd)) != -1) {
        irc_command(irc, "%s\n", command);
      }
    }

    if (FD_ISSET(irc_get_fd(irc), &readfds)) {
      LOG(LOG_LEVEL_DEBUG, "DEBUG: Got some data in the socket\n");
      do {
        message = irc_next_message(irc);
        if (message == NULL) {
          LOG(LOG_LEVEL_DEBUG, "DEBUG: No more message\n");
          if (irc_is_connected(irc) == 0) {
            reconnect = 1;
            break;
          }
        } else {
          LOG(LOG_LEVEL_DEBUG, "DEBUG: Got new message\n");
          // Parse the message
          // Ignore PING, pipe everything else to the output.
          if (strcmp(message->command, "PRIVMSG") == 0) {
            output_message(output_fd, message);
            command_handle_message(irc, message);
          } else if (strcmp(message->command, "PING") == 0) {
            irc_command(irc, "PONG %s", user);
          } else {
            output_message(output_fd, message);
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
  close(output_fd);
  remove(IN_FIFO_PATH);
  remove(OUT_FIFO_PATH);

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

  LOG(LOG_LEVEL_DEBUG, "DEBUG: Waiting for MOTD\n");
  for (int found = 0; found < 1; ) {
    message = wait_for_next_message(irc);
    if (strcmp(message->command, "376") == 0) {
      found = 1;
    }
    irc_message_free(message);
  }

  LOG(LOG_LEVEL_DEBUG, "DEBUG: Sending CAPs\n");
  irc_command(irc, "CAP REQ :twitch.tv/tags twitch.tv/commands");
  for (int found = 0; found < 1; ) {
    message = wait_for_next_message(irc);
    if (strcmp(message->command, "CAP") == 0) {
      found = 1;
    }
    irc_message_free(message);
  }

  // Send JOIN message, wait for NICK list end response.
  LOG(LOG_LEVEL_DEBUG, "DEBUG: Joining the channel\n");
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

void output_message(int file, irc_message_t *message) {
  char buffer[1024] = { 0 };

  if (message->sender == NULL || message->tags == NULL || message->message == NULL) {
    return;
  }

  int len = sprintf(buffer, "%s\t%s", message->tags, message->sender);
  if (message->command != NULL)
    len = len + sprintf(buffer+len, "\t%s", message->command);
  sprintf(buffer+len, "\t%s\n", message->message);

  write(file, buffer, strlen(buffer));
}

void print_usage() {
  fprintf(stderr,
      "Usage: client <user> <password> <channel>\n"
  );
}

int read_line(char *buffer, int size, int fd) {
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

void setup_signals(sigset_t *sigset) {
  // Setup signal interrupts.
  sigset_t mask;
  struct sigaction act;

  memset(&act, 0, sizeof(act));
  act.sa_handler = signal_handler;
  if (sigaction(SIGTERM, &act, 0)) {
    perror("Failed to setup SIGTERM listener.");
    exit(1);
  }
  if (sigaction(SIGINT, &act, 0)) {
    perror("Failed to setup SIGINT listener.");
    exit(1);
  }

  sigemptyset (&mask);
  sigaddset (&mask, SIGTERM);
  sigaddset (&mask, SIGINT);

  if (sigprocmask(SIG_BLOCK, &mask, sigset) < 0) {
    perror ("Failed to sigprocmask");
    exit(1);
  }
}
