#include <stdlib.h>

#include "list.h"

/** 
 * Command handler struct.
 * Matcher function checks whether given IRC message should be handled by the command.
 * Handler function takes current IRC connection, the message that was matched, and
 * does the handling, whatever that might be.
**/
typedef struct {
  int (*matcher)(irc_message_t*);
  void (*handler)(irc_t *, irc_message_t *);
} command_handler_t;

/**
 * List of commands holder.
 */
typedef struct {
  int size;
  command_handler_t **commands;
} command_list_t;

/** Global state **/

static command_list_t commands = { 0 };

/** Public **/

/**
 * Checks if there are any command handlers matching given message, and executes them if needed.
 *
 * @param irc: IRC client.
 * @param message: Message to check.
 *
 * @return: TRUE if at least one command handler matched the message, FALSE otherwise.
 **/
int command_handle_message(irc_t *irc, irc_message_t *message) {
  for (int idx = 0; idx < commands.size; idx++) {
    if (commands.commands[idx]->matcher(message) == 1) {
      commands.commands[idx]->handler(irc, message);
    }
  }
}

/**
 * Register a new command handler.
 *
 * @param matcher: Function that checks if command matches given message.
 * @param handler: Function that handles messages that were matched successfully.
 */
void register_command_handler(int (*matcher)(irc_message_t*), void (*handler)(irc_t *, irc_message_t *)) {
  command_handler_t *command = calloc(1, sizeof(command_handler_t));
  command->matcher = matcher;
  command->handler = handler;

  if (commands.commands == NULL) {
    commands.commands = malloc(1 * sizeof(command_handler_t*));
  } else {
    commands.commands = realloc(commands.commands, commands.size + 1);
  }

  commands.commands[commands.size] = command;
  commands.size = commands.size + 1;
}
