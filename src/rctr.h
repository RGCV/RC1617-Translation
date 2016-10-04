/**
 * RC Translation - RC@IST/UL
 * Common header - RC45179 15'16
 *
 * @author: Rui Ventura (ist181045)
 * @author: Diogo Freitas (ist181586)
 * @author: Sara Azinhal (ist181700)
 */

/* Headers */
#include <stdarg.h>



/* Constants / Macros */
/* Default ports */
#define TCS_DEFAULT_PORT 58050
#define TRS_DEFAULT_PORT 59050

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
#define UTRS_TRANSLATE_REPONSE  "TRR" /* TRS response with translation */
#define UTRS_TRANSLATE_NOTAVAIL "NTA" /* Requested translation not available */

/* TRS-TCS Protocol (in UDP) */
#define SERV_TRSREG_QUERY     "SRG" /* TRS registry query to TCS */
#define SERV_TRSREG_RESPONSE  "SRR" /* TCS response to registry from TRS */
#define SERV_TRSBYE_QUERY     "SUN" /* TRS unregistry query to TCS */
#define SERV_TRSBYE_REPONSE   "SUR" /* TCS response to unregistry from TRS */

#define SERV_DEREGSTATUS_OK  "OK" /* Deregistry successful */
#define SERV_DEREGSTATUS_NOK "NOK" /* Deregistry failed */

/* Error macros */
/* Functional errors */
#define E_GENERIC     0xFF
#define E_INVALIDPORT 0x01

/* GetOpt errors */
#define E_MISSINGARG  0x10
#define E_UNKNOWNOPT  0x11
#define E_DUPARG      0x12



/* Prototypes */
void printHelp (FILE *stream, const char *prog); /* Prints prog help */
void printUsage(FILE *stream, const char *prog); /* Prints prog usage */
int  readArgv  (int argc, char **argv); /* Reads and processes args */
int  eprintf   (const char *format, ...); /* Print to stderr */
