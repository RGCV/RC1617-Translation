/**
 * RC Translation - RC@IST/UL
 * Common header impl - RC45179 16'17
 *
 * @author: Rui Ventura (ist181045)
 * @author: Diogo Freitas (ist181586)
 * @author: Sara Azinhal (ist181700)
 */

#include <errno.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include "rctr.h"

/* Function implementations */
int eprintf(const char *format, ...) {
  int ret;
  va_list args;

  va_start(args, format);
  ret = vfprintf(stderr, format, args);
  va_end(args);

  return ret;
}

ssize_t rwrite(int fd, const void *buffer, size_t size) {
  ssize_t offset = 0;

  while(size > 0) {
    ssize_t nbytes = 0;

    if((nbytes = write(fd, buffer + offset, size)) == -1) {
      if(errno == EPIPE) {
        eprintf("Broken pipe: Connection closed by peer\n");
        return -1;
      }
      else {
        perror("write");
        close(fd);
        exit(E_GENERIC);
      }
    }

    size -= nbytes;
    offset += nbytes;
  }

  return offset;
}

ssize_t rread(int fd, void *buffer, size_t size) {
  ssize_t offset = 0;

  while(size > 0) {
    ssize_t nbytes = 0;

    if((nbytes = read(fd, buffer + offset, size)) == -1) {
      if(errno == EPIPE) {
        eprintf("Broken pipe: Connection closed by peer\n");
        return -1;
      }
      else {
        perror("read");
        close(fd);
        exit(E_GENERIC);
      }
    }

    size -= nbytes;
    offset += nbytes;
  }

  return offset;
}

int udp_send_recv(int sockfd, void *sendbuf, size_t sendlen, size_t sendsize,
    void *recvbuf, size_t recvsize, struct sockaddr *dest_addr,
    socklen_t *addrlen, unsigned long timeout) {
  int err = EXIT_SUCCESS;

  if(recvbuf == NULL) recvbuf = sendbuf;
  if(recvsize <= 0) recvsize = sendsize;

  if(sendto(sockfd, sendbuf, sendlen, 0, dest_addr, *addrlen) == -1) {
    perror("sendto");
    err = -1;
  }

  if(timeout > 0) {
    struct timeval tv;

    tv.tv_sec = timeout;
    tv.tv_usec = 0;
    if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (void *)&tv,
        (socklen_t)sizeof(struct timeval)) == -1) {
      err = errno;
      perror("setsockopt");
    }
  }

  if(!err && recvfrom(sockfd, recvbuf, recvsize, 0, dest_addr, addrlen) == -1) {
    perror("recvfrom");
    err = -1;
  }

  return err;
}
