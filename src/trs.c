/**
 * RC Translation - RC@IST/UL
 * Common header - RC45179 15'16
 *
 * @author: Rui Ventura (ist181045)
 * @author: Diogo Freitas (ist181586)
 * @author: Sara Azinhal (ist181700)
 */

#include <errno.h>
#include <limits.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "rctr.h"

/* Global variables */
volatile int interrupted = 0; /*  */

struct hostent *TCSname; /* TCS hostent struct */
unsigned short TRSport, TCSport; /* TRS and TCS ports */
char TRSlanguage[LANG_MAX_LEN]; /* The language TRS provides translation for */



/* TRS main function */
int main(int argc, char **argv) {
  int ret; /* readArgv return code */
  int tcsfd; /* TCS UDP socket fd */
  bool shouldRun = true; /* should the loop be running flag */
  struct sockaddr_in sockaddr; /* sockaddr */
  socklen_t addrlen; /* Length of sockaddr */

  /* Setup defaults */
  TCSname = gethostbyname("0.0.0.0");
  TCSport = TCS_DEFAULT_PORT;
  TRSport = TRS_DEFAULT_PORT;

  if((ret = readArgv(argc, argv))) {
    printUsage(stderr, argv[0]);
    exit(ret);
  }

  return EXIT_SUCCESS;
}



/* Function implementations */
void printHelp(FILE *stream, const char *prog) {
  fprintf(stream, "RC Translation - TRS (Translation Contact Server)\n");
  printUsage(stream, prog);
  fprintf(stream,
    "Options:\n"

    "\t-h   Shows this help message and exits\n"
    "\t-n   The TCS\' hostname, where TCSname is an IPv4 address\n"
    "\t     or a name (default: localhost)\n"
    "\t-p   The TRS\' port, in the range 0-65535 (default: TRSport = %hu)\n"
    "\t-e   The TCS\'s port, in the range 0-65535 (default: TCSport = %hu)\n",
  TRS_DEFAULT_PORT, TCS_DEFAULT_PORT);
}

void printUsage(FILE *stream, const char *prog) {
  fprintf(stream, "Usage: %s <language> [-p TRSport] [-n TCSname] "
    "[-e TCSport]\n", prog);
}

int readArgv(int argc, char **argv) {
  char c;
  int err = EXIT_SUCCESS;
  bool TRSlangflg = false,
    TRSportflg = false,
    TCSnameflg = false,
    TCSportflg = false;

  while(optind < argc && !err) {
    /* Handle options */
    if((c = (char)getopt(argc, argv, ":hp:n:e:")) != -1) {
      switch(c) {
        case 'h': /* Print Help & Usage */
          printHelp(stdout, argv[0]);
          exit(EXIT_SUCCESS);
        case 'n': /* TCS hostname */
          if(!TCSnameflg) {
            if((TCSname = gethostbyname(optarg)) == NULL) {
              herror("gethostbyname");
              err = E_GENERIC;
            }
            else TCSnameflg = true;
          }
          else {
            eprintf("Host already assigned: %s\n", TCSname->h_name[0]);
            err = E_DUPOPT;
          }
          break;
        case 'p': /* TRSport number */
        case 'e': /* TCSport number */ {
          long int val;
          char *endptr;

          if((c == 'p' && TRSportflg) || (c == 'e' && TCSportflg)) {
            eprintf("%s Port was already assigned: %hu\n",
              TCSportflg ? "TCS" : "TRS", TCSportflg ? TCSport : TRSport);
            err = E_DUPOPT;
          }
          else {
            val = strtol(optarg, &endptr, 10);
            if((errno == ERANGE && (val == LONG_MIN || val == LONG_MAX))
                || (errno != 0 && val == 0)) {
              perror("strtol");
              err = E_GENERIC;
            } /* Overflow or an invalid value */
            else if(val < 0 || val > USHRT_MAX || endptr == optarg
                || *endptr != '\0') {
              fprintf(stderr, "Invalid port: %s\n", optarg);
              err = E_INVALIDPORT;
            } /* Out of port (0-65535) range, blank or has alpha characters */
            else {
              switch (c) {
                case 'p':
                  TRSport = (unsigned short)val;
                  TRSportflg = true;
                  break;
                case 'e':
                  TCSport = (unsigned short)val;
                  TCSportflg = true;
              }
            } /* All good! */
          }
          break;
        }
        case ':': /* Opt requires arg */
          eprintf("Option \'-%c\' requires an argument\n", optopt);
          err = E_MISSINGARG;
          break;
        case '?': /* Unknown opt */
          eprintf("Unknown option: \'-%c\'\n", optopt);
          err = E_UNKNOWNOPT;
      }
    }
    /* Else, if the argument isn't null, assign it to TRSlanguage */
    else if(argv[optind] != NULL) {
      strncpy(TRSlanguage, argv[optind], LANG_MAX_LEN - 1);
      TRSlanguage[LANG_MAX_LEN - 1] = '\0';
      TRSlangflg = true;
      ++optind;
    }
  }

  /* If a language wasn't passed */
  if(!err && !TRSlangflg) {
    eprintf("%s: Missing language argument\n", argv[0]);
    err = E_MISSINGARG;
  }

  return err;
}
