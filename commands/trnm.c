#include <stdio.h>
#include <string.h>

#include "list.h"
#include "tags.h"
#include "../irc.h"

int trnm_match(irc_message_t *message) {
  if (strcmp("$trnm", message->message) == 0) {
    return 1;
  }

  return 0;
}

void trnm_handle(irc_t *irc, irc_message_t *message) {
  char info[] = 
    "I'm participating in DyingCamel's SuperFlight tournament. "
    "You can check the rules and current leaderboard here: "
    "https://docs.google.com/spreadsheets/d/1OERHgD19SQRSOfp-XRzzY1cZBmdxc-O0FMGNXKxOqvE/edit#gid=0";
  irc_command(irc, "PRIVMSG %s :%s", message->recipient, info);
}
