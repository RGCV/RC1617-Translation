/**
 * RC Translation - RC@IST/UL
 * Common header impl - RC45179 15'16
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

ssize_t rwrite(int fd, char *buffer, ssize_t size) {
  ssize_t offset = 0;

  while(size > 0) {
    ssize_t nbytes = 0;

    if((nbytes = write(fd, &buffer[offset], size)) == -1) {
      if(errno == EPIPE) {
        eprintf("Broken pipe: Connection closed\n");
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

ssize_t rread(int fd, char *buffer, ssize_t size) {
  ssize_t offset = 0;

  while(size > 0) {
    ssize_t nbytes = 0;

    if((nbytes = read(fd, &buffer[offset], size)) == -1) {
      if(errno == EPIPE) {
        eprintf("Broken pipe: Connection closed\n");
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

int udp_send_recv(int sockfd, void *buf, size_t len, size_t size,
    struct sockaddr *dest_addr, socklen_t *addrlen, unsigned long delay) {
  int err = EXIT_SUCCESS;

  if(sendto(sockfd, buf, len, 0, dest_addr, *addrlen) == -1) {
    err = errno;
    perror("sendto");
  }

  if(delay > 0) {
    struct timeval tv;

    tv.tv_sec = delay;
    tv.tv_usec = 0;
    if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (void *)&tv,
        (socklen_t)sizeof(struct timeval)) == -1) {
      err = errno;
      perror("setsockopt");
    }
  }

  if(!err && recvfrom(sockfd, buf, size, 0, dest_addr, addrlen) == -1) {
    err = errno;
    perror("recvfrom");
  }

  return err;
}
