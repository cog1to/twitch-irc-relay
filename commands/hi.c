#include <stdio.h>
#include <string.h>

#include "list.h"
#include "../irc.h"


int hi_match(irc_message_t *message) {
  if (strcmp("$hi", message->message) == 0) {
    return 1;
  }

  return 0;
}

void hi_handle(irc_t *irc, irc_message_t *message) {
  irc_command(irc, "PRIVMSG %s :hi, %s", message->recipient, message->sender);
}
