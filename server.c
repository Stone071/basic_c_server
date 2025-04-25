// +------------------------------------+
// | LEARNING SOCKETS!                  |
// | THE SERVER                         |
// +------------------------------------+
#include <sys/socket.h>
#include <stdio.h>

#define PROTOCOL_TCP 6
#define PORT 8080

// ############# NOTES ##############################################
// socket() creates a socket
// connect() connects a socket to a remote socket address
// bind() binds a socket to a local socket address.
// listen() tells the socket that new connections shall be accepted.
// accept() is used to get a new socket with a new incoming connection.

// Let's create a socket and return the handle to it.
// AF_LOCAL is used because we are going to use localhost.

// SOCK_STREAM can be bitwise ORed with SOCK_NONBLOCK or SOCK_CLOEXEC
// SOCK_STREAM is creates a full-duplex byte stream. A stream socket must be in 
// a "connected" state before data may be sent or received on it. read(2), write(2),
// send(2), recv(2) and other variants may be used to communicate on a SOCK_STREAM.

// Protocols are described in protocols(3) which directs you to /etc/protocols.
// 6 is TCP
// ##################################################################


int main(void)
{
  int sockfd;
  
  sockfd = socket(AF_LOCAL, SOCK_STREAM, PROTOCOL_TCP);
  if (sockfd == -1)
  {
    // Print out the human readable error and string source
    perror("Open socket failed\n");
  }
}