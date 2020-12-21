#ifndef IRC_HEADER
#define IRC_HEADER

/* IRC client instance */
typedef struct irc_t irc_t;

/* IRC message data structure */
typedef struct irc_message_t {
  char *tags;
  char *sender;
  char *command;
  char *recipient;
  char *message;
} irc_message_t;

/**
 * Creates a new IRC client instance.
 *
 * @param connection: Socket file descriptor.
 *
 * @return: A new client instance.
 **/
irc_t *irc_init(int connection);

/**
 * Checks whether instance is currently connected.
 *
 * @param irc: IRC client.
 *
 * @return: Flag indicating whether client connection is opened.
 **/
int irc_is_connected(irc_t *irc);

/**
 * Sends a new command through the IRC connection.
 *
 * @param irc: IRC client.
 * @param fmt: Command format string.
 * @param args: Command parameters.
 *
 * @return: Number of bytes sent, if sending is successfull, -1 otherwise.
 **/
int irc_command(irc_t *irc, const char *fmt, ...);

/**
 * Sends raw data to the IRC connection.
 *
 * @param irc: IRC client.
 * @param str: Data to send.
 *
 * @return: Number of bytes sent, if sending is successfull, -1 otherwise.
 */
int irc_command_literal(irc_t *irc, char *str);

/**
 * Waits for the next message from IRC connection. Block the thread while doing so.
 *
 * @param irc: IRC client.
 *
 * @return: Pointer to a new message, or NULL in case of an error.
 */
irc_message_t *irc_next_message(irc_t *irc);

/**
 * Disconnects the IRC client and frees the memory occupied by it.
 *
 * @param irc: IRC client to deallocate.
 **/
void irc_free(irc_t *irc);

/**
 * Deallocates message structure.
 *
 * @param message: Message to deallocate.
 **/
void irc_message_free(irc_message_t *message);

#endif
