#include <dbus/dbus.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "dbus.h"

struct dbus_server_t {
  DBusConnection *conn;
  DBusWatch *watch;
  int fd;
  const char *interface;
  const char *type;
};

dbus_bool_t add_watch(DBusWatch *w, void *data);
void remove_watch(DBusWatch *w, void *data);
void toggle_watch(DBusWatch *w, void *data);

/** Interface **/

dbus_server_t *dbus_server_init(const char *dest, const char *interface, const char *type) {
  DBusConnection *conn;
  DBusError err;

  // Initialize the error struct
  dbus_error_init(&err);

  // Try to establish a connection to the DBus daemon.
  conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
  if (dbus_error_is_set(&err)) {
    fprintf(stderr, "Failed to establish a connection to the DBus: %s\n", err.message);
    dbus_error_free(&err);
  }
  if (conn == NULL) {
    return NULL;
  }

  // Request a path/name.
  int result = dbus_bus_request_name(conn, dest, DBUS_NAME_FLAG_REPLACE_EXISTING, &err);
  if (dbus_error_is_set(&err)) {
    fprintf(stderr, "Failed to request a server name: %s\n", err.message);
    dbus_error_free(&err);
    return NULL;
  }

  // Add a match rule.
  char match_string[128] = {0};
  sprintf(match_string, "type='signal',interface='%s'", interface);
  dbus_bus_add_match(conn, match_string, &err);
  dbus_connection_flush(conn);
  if (dbus_error_is_set(&err)) {
    fprintf(stderr, "Failed to set a match: %s\n", err.message);
    dbus_error_free(&err);
    return NULL;
  }

  // Initialize and return an instance of a server.
  dbus_server_t *server = malloc(sizeof(dbus_server_t));
  if (server == NULL) {
    return NULL; // Failed to allocate enough memory
  }

  // Set match functions.
  if (!dbus_connection_set_watch_functions(conn, add_watch, remove_watch, toggle_watch, server, NULL)) {
    fprintf(stderr, "Failed to set watch functions\n");
    free(server);
    return NULL;
  }

  server->conn = conn;
  server->interface = interface;
  server->type = type;
  return server;
}

void dbus_server_deinit(dbus_server_t *server) {
  if (server == NULL) {
    return;
  }

  free(server);
}

int dbus_server_get_fd(dbus_server_t *server) {
  return server->fd;
}

void dbus_server_get_signal(dbus_server_t *server, char **signal) {
  if (server->watch == NULL) { return; }
  dbus_watch_handle(server->watch, DBUS_WATCH_READABLE);

  DBusDispatchStatus dispatch_rc = dbus_connection_get_dispatch_status(server->conn);
  if (DBUS_DISPATCH_DATA_REMAINS != dispatch_rc) {
    fprintf(stderr, "ERROR recv no message in queue\n");
    return;
  }

  while(DBUS_DISPATCH_DATA_REMAINS == dispatch_rc) {
    DBusMessage* msg = dbus_connection_pop_message(server->conn);
    if (msg == NULL) {
      fprintf(stderr, "ERROR recv pending check FAILED: remains but no message borrowed.\n");
      return;
    }

    const char *iface = dbus_message_get_interface(msg);
    const char *member = dbus_message_get_member(msg);
    if (dbus_message_is_signal(msg, server->interface, server->type)) {
      DBusMessageIter args;
      if (!dbus_message_iter_init(msg, &args)) {
        fprintf(stderr, "No arguments in a message\n");
      } else if (DBUS_TYPE_STRING != dbus_message_iter_get_arg_type(&args)) {
        fprintf(stderr, "Expecting string for an argument\n");
      } else {
        fprintf(stderr, "Found a message %s, %s, returning\n", iface, member);
        dbus_message_iter_get_basic(&args, signal);
        return;
      }
    } else {
      fprintf(stderr, "Unsupported message type: %s, %s\n", iface, member);
    }

    dbus_message_unref(msg);
    dispatch_rc = dbus_connection_get_dispatch_status(server->conn);
  }
}

void dbus_server_send_signal(
  dbus_server_t *server,
  const char *path,
  const char *interface,
  const char *name,
  const char *message
) {
  DBusMessage *msg;
  DBusMessageIter args;

  msg = dbus_message_new_signal(
    path,
    interface,
    name
  );

  if (msg == NULL) {
    fprintf(stderr, "DBUS: Failed to allocate a message.\n");
    return;
  }

  dbus_message_iter_init_append(msg, &args);
  if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &message)) {
    fprintf(stderr, "DBUS: Failed to append message body.\n");
    dbus_message_unref(msg);
    return;
  }

  int id = 0;
  if (!dbus_connection_send(server->conn, msg, &id)) {
    fprintf(stderr, "DBUS: Failed to send the signal.\n");
    dbus_message_unref(msg);
    return;
  }

  dbus_connection_flush(server->conn);
  dbus_message_unref(msg);
}

/** Private **/

dbus_bool_t add_watch(DBusWatch *w, void *data) {
  if (!dbus_watch_get_enabled(w)) {
    return TRUE;
  }

  dbus_server_t *server = (dbus_server_t *)data;
  int fd = dbus_watch_get_unix_fd(w);
  server->watch = w;
  server->fd = fd;

  return TRUE;
}

void remove_watch(DBusWatch *w, void *data) {
  dbus_server_t *server = (dbus_server_t *)data;
  server->fd = -1;
  server->watch = NULL;
}

void toggle_watch(DBusWatch *w, void *data) {
  if (dbus_watch_get_enabled(w)) {
    add_watch(w, data);
  } else {
    remove_watch(w, data);
  }
}
