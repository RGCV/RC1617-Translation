/**
 * RC Translation - RC@IST/UL
 * Common header - RC45179 15'16
 *
 * @author: Rui Ventura (ist181045)
 * @author: Diogo Freitas (ist181586)
 * @author: Sara Azinhal (ist181700)
 */

#ifndef _RCTR_H
#define _RCTR_H

/* Required headers for external definitions */
#include <stdio.h>
#include <sys/socket.h>

/* Constants / Macros */
/* File name max length */
#define FILE_MAX_LEN 0xFF /* 255 */

/* Language length */
#define LANG_MAX_LEN 21

/* Line length */
#define LINE_MAX_LEN 0x1FF /* 512 */

/* Minimum port value */
#define PORT_MIN 1024

/* Packet size */
#define PCKT_MAX_SIZE 0x7FF /* 2047 */

/* Protocol message length */
#define PMSG_MAX_LEN 0x7F /* 127 */

/* Default ports */
#define TCS_DEFAULT_PORT 58050
#define TRS_DEFAULT_PORT 59000

/* General protocol error code strings */
#define REQ_NAVAIL "EOF" /* Request failed, bad spelling, wasn't found, ... */
#define REQ_ERROR "ERR" /* Protocol syntax error */

/* User-TCS Protocol (in UDP) */
#define UTCS_LANG_REQ "ULQ" /* 'list' request */
#define UTCS_LANG_RSP "ULR" /* 'list' response */

#define UTCS_NAMESERV_REQ "UNQ" /* 'request' request */
#define UTCS_NAMESERV_RSP "UNR" /* 'request' response */

/* User-TRS Protocol (in TCP) */
#define UTRS_TRANSLATE_REQ      "TRQ" /* after 'request' to TCS */
#define UTRS_TRANSLATE_RSP      "TRR" /* TRS response with translation */
#define UTRS_TRANSLATE_NAVAIL "NTA" /* Requested translation not available */

/* TRS-TCS Protocol (in UDP) */
#define SERV_TRSREG_REQ "SRG" /* TRS registry request to TCS */
#define SERV_TRSREG_RSP "SRR" /* TCS response to registry from TRS */
#define SERV_TRSBYE_REQ "SUN" /* TRS unregistry request to TCS */
#define SERV_TRSBYE_RSP "SUR" /* TCS response to unregistry from TRS */

#define SERV_STATUS_OK  "OK" /* Registry/Deregistry successful */
#define SERV_STATUS_NOK "NOK" /* Registry/Deregistry failed */

/* Error macros */
/* Functional errors */
#define E_GENERIC     0xFF /* 255: generic error (-1 signed short) */

/* GetOpt errors */
#define E_INVALIDPORT 0x10 /* 16: Port given as arg was invalid */
#define E_MISSINGARG  0x11 /* 17: Missing argument for opt that required one */
#define E_UNKNOWNOPT  0x12 /* 18: Option unrecognized */
#define E_DUPOPT      0x13 /* 19: Duplicate option */

/* Protocol errors */
#define E_PROTREQERROR 0x20 /* 32: Protocol message's syntax was wrong */

/* Prototypes */
void printHelp(FILE *stream, const char *prog); /* Prints prog help */

void printUsage(FILE *stream, const char *prog); /* Prints prog usage */

int readArgv(int argc, char **argv); /* Reads and processes args */

int eprintf(const char *format, ...); /* Print to stderr */

ssize_t rwrite(int fd, const void *buffer, size_t size); /* Persistent write */

ssize_t rread(int fd, void *buffer, size_t size); /* Persistent read */

int udp_send_recv(int sockfd, void *sendbuf, size_t sendlen, size_t sendsize,
  void *recvbuf, size_t recvsize, struct sockaddr *dest_addr,
  socklen_t *addrlen, unsigned long timeout);
/* Send len bytes in buf and receive at most size bytes in buf */

#endif /* _RCTR_H */
