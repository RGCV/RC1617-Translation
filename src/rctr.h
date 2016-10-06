/**
 * RC Translation - RC@IST/UL
 * Common header - RC45179 15'16
 *
 * @author: Rui Ventura (ist181045)
 * @author: Diogo Freitas (ist181586)
 * @author: Sara Azinhal (ist181700)
 */

/* Constants / Macros */
/* Default ports */
#define TCS_DEFAULT_PORT 58050
#define TRS_DEFAULT_PORT 59050

/* Packet size */
#define PCKT_SIZE_MAX 2047

/* General protocol error code strings */
#define QUERY_INVALID "EOF" /* Query failed, bad spelling, wasn't found, ... */
#define QUERY_BADFORM "ERR" /* Protocol syntax error */

/* User-TCS Protocol (in UDP) */
#define UTCS_LANG_QUERY    "ULQ" /* 'list' query */
#define UTCS_LANG_RESPONSE "ULR" /* 'list' response */

#define UTCS_NAMESERV_QUERY    "UNQ" /* 'request' query */
#define UTCS_NAMESERV_RESPONSE "UNR" /* 'request' response */

/* User-TRS Protocol (in TCP) */
#define UTRS_TRANSLATE_QUERY    "TRQ" /* after 'request' to TCS */
#define UTRS_TRANSLATE_RESPONSE  "TRR" /* TRS response with translation */
#define UTRS_TRANSLATE_NOTAVAIL "NTA" /* Requested translation not available */

/* TRS-TCS Protocol (in UDP) */
#define SERV_TRSREG_QUERY     "SRG" /* TRS registry query to TCS */
#define SERV_TRSREG_RESPONSE  "SRR" /* TCS response to registry from TRS */
#define SERV_TRSBYE_QUERY     "SUN" /* TRS unregistry query to TCS */
#define SERV_TRSBYE_RESPONSE   "SUR" /* TCS response to unregistry from TRS */

#define SERV_DEREGSTATUS_OK  "OK" /* Deregistry successful */
#define SERV_DEREGSTATUS_NOK "NOK" /* Deregistry failed */

/* Error macros */
/* Functional errors */
#define E_GENERIC     0xFF /* Short-sized signed -1, unsigned 255 */

/* GetOpt errors */
#define E_INVALIDPORT 0x10 /* Port given as arg was invalid */
#define E_MISSINGARG  0x11 /* Missing argument for option that required one */
#define E_UNKNOWNOPT  0x12 /* Option unrecognized */
#define E_DUPOPT      0x13 /* Duplicate option */

/* Protocol errors */
#define E_PROTINVALID  0x20 /* Valid response not found, wasn't available, .. */
#define E_PROTQBADFORM 0x21 /* Protocol message's syntax was wrong */

/* Prototypes */
void printHelp (FILE *stream, const char *prog); /* Prints prog help */

void printUsage(FILE *stream, const char *prog); /* Prints prog usage */

int  readArgv  (int argc, char **argv); /* Reads and processes args */

int  eprintf   (const char *format, ...); /* Print to stderr */

ssize_t rwrite(int fd, char *buffer, ssize_t size); /* Persistant write */

ssize_t rread(int fd, char *buffer, ssize_t size); /* Persistant read */

int udp_send_recv(int sockfd, void *buf, size_t len, size_t size,
  struct sockaddr *dest_addr, socklen_t *addrlen, unsigned long delay);
/* Send len bytes in buf and receive at most size bytes in buf */
