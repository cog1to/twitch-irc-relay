#ifndef COMMANDS_HEADER
#define COMMANDS_HEADER

#include "../irc.h"

/**
 * Checks if there are any command handlers matching given message, and executes them if needed.
 *
 * @param irc: IRC client.
 * @param message: Message to check.
 *
 * @return: TRUE if at least one command handler matched the message, FALSE otherwise.
 **/
int command_handle_message(irc_t *irc, irc_message_t *message);

/**
 * Register a new command handler.
 *
 * @param matcher: Function that checks if command matches given message.
 * @param handler: Function that handles messages that were matched successfully.
 */
void register_command_handler(
  int (*matcher)(irc_message_t*),
  void (*handler)(irc_t *, irc_message_t *)
);

#define REGISTER(command) register_command_handler(command ## _match, command ## _handle);

#define COMMAND(command) \
  extern int command ## _match(irc_message_t *); \
  extern void command ## _handle(irc_t *, irc_message_t *);

#endif
