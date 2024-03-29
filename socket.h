#ifndef SOCKET_HEADER
#define SOCKET_HEADER

/**
 * Opens a socket connection to given server:port.
 *
 * @param host: Host name.
 * @param port: Port number.
 *
 * @return: A file descriptor if connection is opened.
 **/
int sock_connect(char *host, int port);

/**
 * Closes open socket connection
 *
 * @param socket: Socket's file descriptor;
 *
 * @return: 0 in case of success, -1 in case of an error. Check errno for
 * the specific error.
 */
int sock_close(int socket);

/**
 * Reads a portion of data from a socket.
 *
 * @param socket: Socket's file descriptor.
 * @param data: A byte buffer to read into.
 * @param size: Buffer's size.
 *
 * @return: Number of bytes read, or -1 in case of an error.
 **/
int sock_receive(int socket, char *data, short size);

/**
 * Reads a portion of data from a socket while blocking the caller thread until
 * some amount of data is read.
 *
 * @param socket: Socket's file descriptor.
 * @param data: A byte buffer to read into.
 * @param size: Buffer's size.
 *
 * @return: Number of bytes read, or -1 in case of an error.
 **/
int sock_block_receive(int socket, char *data, short size);

/**
 * Sends data into a socket.
 *
 * @param socket: Socket's file descriptor.
 * @param data: Data buffer to read data from.
 * @param size: Size of the data chunk to send.
 *
 * @return: 0 if send is successfull, -1 in case of an error.
 **/
int sock_send(int socket, char *data, short size);

/**
 * Peeks into the socket without destroying data.
 *
 * @param socket: Socket's file descriptor.
 *
 * @return: 0 if peek is successfull, -1 in case of an error.
 **/
int sock_peek(int socket);

#endif
