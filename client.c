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
#include "dbus.h"

/** Commands **/

COMMAND(hi)

void register_commands() {
  REGISTER(hi)
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
 * Sends the message to the DBUS.
 *
 * @param server: DBUS connection holder.
 * @param message: Message to send.
 **/
void send_message_to_dbus(dbus_server_t *server, irc_message_t *message);

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

/**
 * Quote-escapes input string and puts it into output string.
 * If resulting string is larger than output, the string is truncated.
 *
 * @param in: Input string.
 * @param out: Output string.
 * @param outsize: Byte-length of the output buffer.
 */
void string_quote_escape(char *in, char *out, int outsize);

/** Constants **/

/* Input data path.  */
char const * const IN_FIFO_PATH = "/tmp/twitch-bot-in";
char const * const OUT_FIFO_PATH = "/tmp/twitch-bot-out";

/* DBUS connection settings. */
char const * const DBUS_NAME = "ru.aint.twitch.chat";
char const * const DBUS_INTERFACE = "ru.aint.twitch.signal";
char const * const DBUS_SIGNAL = "Command";

/* Input message buffer size. */
int const INPUT_BUFFER_SIZE = 1024;

typedef enum {
  IO_FIFO,
  IO_STD,
  IO_DBUS
} io_t;

/** Main **/

int main(int argc, char **argv) {
  char *server = "irc.chat.twitch.tv";
  int port = 6667;
  io_t io_type = IO_STD;

  char *user, *password, *channel, *io;
  if (argc < 4) {
    print_usage();
    exit(0);
  }

  user = argv[1];
  password = argv[2];
  channel = argv[3];

  // IO type.
  if (argc > 4) {
    if (strcmp("-f", argv[4]) == 0) {
      io_type = IO_FIFO;
    } else if (strcmp("-d", argv[4]) == 0) {
      io_type = IO_DBUS;
    }
  }

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

  LOG(LOG_LEVEL_DEBUG, "DEBUG: Setting up the I/O\n");

  // I/O setup.
  int input_fd = 0, output_fd = 1;
  char command[INPUT_BUFFER_SIZE];
  dbus_server_t *dbus = NULL;

  if (io_type == IO_FIFO) {
    int error = mkfifo(IN_FIFO_PATH, S_IRUSR | S_IWUSR);
    if (error != 0) {
      perror("Failed to create a FIFO");
      exit(-1);
    }

    // Input feed.
    input_fd = open(IN_FIFO_PATH, O_RDWR);

    // Output.
    error = mkfifo(OUT_FIFO_PATH, S_IRUSR | S_IWUSR);
    if (error != 0) {
      perror("Failed to create a FIFO");
      exit(-1);
    }

    // Output feed.
    output_fd = open(OUT_FIFO_PATH, O_RDWR);
  } else if (io_type == IO_DBUS) {
    dbus = dbus_server_init(DBUS_NAME, DBUS_INTERFACE, DBUS_SIGNAL);
    if (dbus == NULL) {
      perror("Failed to establish a connection to DBUS");
      exit(-1);
    }
  }

  // We want to wait for either command input or socket data.
  fd_set readfds;

  // Add signal interruptors.
  sigset_t orig_mask;
  setup_signals(&orig_mask);

  // Read timeout spec.
  struct timespec timeout = {
    .tv_sec = 30,
    .tv_nsec = 0
  };

  LOG(LOG_LEVEL_DEBUG, "DEBUG: Entering the main loop\n");

  // Main loop (will interrupt on SIGTERM or SIGINT (CTRL+C))
  int reconnect = 0;
  while (terminate == 0) {
    if (reconnect) {
      LOG(LOG_LEVEL_DEBUG, "DEBUG: Reconnecting\n");
      irc_free(irc);
      irc = do_connect(server, port, user, password, channel);
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

    timeout.tv_sec = 20;
    timeout.tv_nsec = 0;

    int activity = pselect(maxfd + 1, &readfds, NULL, NULL, &timeout, &orig_mask);
    if (activity == -1) {
      perror("Error while waiting for the input");
      break;
    } else if (activity == 0) {
      LOG(LOG_LEVEL_DEBUG, "DEBUG: Got pselect timeout\n");
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
        } else {
          LOG(LOG_LEVEL_DEBUG, "DEBUG: Got new message\n");
          // Parse the message
          // Ignore PING, pipe everything else to the output.
          if (strcmp(message->command, "PRIVMSG") == 0) {
            if (io_type == IO_DBUS && dbus != NULL) {
              send_message_to_dbus(dbus, message);
            } else {
              output_message(output_fd, message);
              command_handle_message(irc, message);
            }
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


    if (irc_is_connected(irc) == 0) {
      reconnect = 1;
    }
  }

  // Clean up.
  if (irc != NULL) {
    irc_free(irc);
  }
  if (dbus != NULL) {
    dbus_server_deinit(dbus);
  }

  // Close the streams.
  fprintf(stdout, "%d", EOF);
  if (io_type == IO_FIFO) {
    close(input_fd);
    close(output_fd);
    remove(IN_FIFO_PATH);
    remove(OUT_FIFO_PATH);
  }

  // Exit without errors.
  return 0;
}

/** Helpers **/

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
  irc_command(irc, "USER %s", user);

  LOG(LOG_LEVEL_DEBUG, "DEBUG: Waiting for RPL_WELCOME\n");
  for (int found = 0; found < 1; ) {
    message = wait_for_next_message(irc);
    if (strcmp(message->command, "001") == 0) {
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

void serialize_message(irc_message_t *message, char *buffer) {
  char escaped_message[1024] = { 0 };

  if (message->sender == NULL || message->tags == NULL || message->message == NULL) {
    return;
  }

  int len = sprintf(buffer, "{\"tags\":\"%s\",\"sender\":\"%s\"", message->tags, message->sender);
  if (message->command != NULL)
    len = len + sprintf(buffer+len, ",\"command\":\"%s\"", message->command);

  // Quote-escape message first, then append it to the output.
  string_quote_escape(message->message, escaped_message, 1010 - len);
  sprintf(buffer+len, ",\"message\":\"%s\"}\n", escaped_message);
}

void send_message_to_dbus(dbus_server_t *server, irc_message_t *message) {
  char buffer[1024] = { 0 };
  serialize_message(message, buffer);
  dbus_server_send_signal(
    server,
    "/ru/aint/twitch/signal",
    "ru.aint.twitch.signal",
    "Message",
    buffer
  );
}

void output_message(int file, irc_message_t *message) {
  char buffer[1024] = { 0 };
  serialize_message(message, buffer);
  write(file, buffer, strlen(buffer));
}

void print_usage() {
  fprintf(stderr,
      "Usage: twitch-bot <user> <password> <channel> [-f|-s|-d]\n  -f: Use named pipes instead of STD for input and output.\n  -s: [Default] Use standard input/output pipes for input and output.\n  -d: Use DBUS to send and receive chat messages and commands.\n"
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

void string_quote_escape(char *in, char *out, int outsize) {
  int out_idx = 0;
  for (int idx = 0; idx < strlen(in) && out_idx < outsize - 1; idx++) {
    if (in[idx] == '"') {
      out[out_idx++] = '\\';
    }

    if (out_idx < (outsize - 1)) {
      out[out_idx] = in[idx];
      out_idx += 1;
    }
  }

  out[out_idx] = '\0';
}
