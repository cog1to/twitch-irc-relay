#ifndef H_DBUS_LISTENER
#define H_DBUS_LISTENER

typedef struct dbus_server_t dbus_server_t;

dbus_server_t *dbus_server_init(const char *dest, const char *interface, const char *type);

void dbus_server_deinit(dbus_server_t *server);

int dbus_server_get_fd(dbus_server_t *server);

void dbus_server_get_signal(dbus_server_t *server, char **signal);

void dbus_server_send_signal(
  dbus_server_t *server,
  const char *path,
  const char *interface,
  const char *name,
  const char *message
);

#endif
