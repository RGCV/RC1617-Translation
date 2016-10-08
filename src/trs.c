/**
 * RC Translation - RC@IST/UL
 * Common header - RC45179 15'16
 *
 * @author: Rui Ventura (ist181045)
 * @author: Diogo Freitas (ist181586)
 * @author: Sara Azinhal (ist181700)
 */

#include <arpa/inet.h>
#include <errno.h>
#include <limits.h>
#include <netdb.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "rctr.h" /* common header */



/* Constants */
#define FILE_TEXT_TR "text_translation.txt"
#define FILE_FILE_TR "file_translation.txt"


/* Global variables */
volatile bool interrupted = false; /* flag to determine if SIGINT was issued */

struct hostent *TCSname; /* TCS hostent struct */
unsigned short TRSport, TCSport; /* TRS and TCS ports */
char TRSlanguage[LANG_MAX_LEN]; /* The language TRS provides translation for */



/* Function prototypes */
void handle_signal(int signo); /* Signal handling function*/



/* TRS main function */
int main(int argc, char **argv) {
  int ret; /* readArgv return code */
  int tcsfd; /* TCS UDP socket fd */
  bool shouldRun = true; /* should the loop be running flag */

  FILE *filesfp, *textfp; /* translation files streams */

  struct sockaddr_in sockaddr; /* sockaddr */
  socklen_t addrlen; /* Length of sockaddr */

  /* Signal handling */
  struct sigaction sa;
  sa.sa_handler = handle_signal; /* signal handler function */
  sigemptyset(&sa.sa_mask); /* empty sa struct's signal mask */
  sa.sa_flags = SA_RESTART; /* interrupted functions will restart */

  /* if signal action registry fails */
  if(sigaction(SIGINT, &sa, NULL) == -1) {
    perror("sigaction");
    exit(E_GENERIC);
  }

  /* Setup defaults */
  TCSname = gethostbyname(name);
  TCSport = TCS_DEFAULT_PORT;
  TRSport = TRS_DEFAULT_PORT;

  /* Read and parse command line args */
  if((ret = readArgv(argc, argv))) {
    printUsage(stderr, argv[0]);
    exit(ret);
  }

  /* Create socket for communication */
  if((tcsfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
    perror("socket");
    exit(E_GENERIC);
  }

  memset((void *)&sockaddr, (int)'\0', sizeof(struct sockaddr_in));
  sockaddr.sin_family = AF_INET;
  inet_aton(TCSname->h_addr_list[0], (struct in_addr *)&sockaddr.sin_addr.s_addr);
  sockaddr.sin_port = htons(TRSport);
  addrlen = sizeof(sockaddr);

  /* Bind the socket to TRSport */
  if(bind(tcsfd, (struct sockaddr *)&sockaddr, addrlen) == -1) {
    perror("bind");
    exit(E_GENERIC);
  }

  /* Try and open translation files */
  if((textfp = fopen(FILE_TEXT_TR, "r")) == NULL
      || (filesfp = fopen(FILE_FILE_TR, "r")) == NULL) {
    perror("fopen");
    exit(E_GENERIC);
  }

  /* TRS main loop */
  while(shouldRun) {
    if(interrupted) {
      eprintf("\rInterrupted!\n");
      shouldRun = false;
    }
    else {

    }
  }

  return EXIT_SUCCESS;
}



/* Function implementations */
void handle_signal(int signo) {
  switch(signo) {
    case SIGINT: interrupted = true;
    default: break;
  }
}

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
