/**
 * RC Translation
 * TCS (Translation Central Server) - RC45179 15'16
 *
 * @author: Rui Ventura (ist181045)
 * @author: Diogo Freitas (ist181586)
 * @author: Sara Azinhal (ist181700)
 */

#include <errno.h>
#include <limits.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "rctr.h"
#include "trs_list.h"



/* Global variables */
unsigned short TCSport; /* Port to bind TCS to */



/* Client main function */
int main(int argc, char **argv) {
  int ret; /* readArgv return code */
  int sockfd; /* TCS UDP socket fd */
  bool shouldRun = false;
  struct sockaddr_in sockaddr; /* sockaddr */
  socklen_t addrlen;

  trs_list_t *trs_list;

  TCSport = TCS_DEFAULT_PORT;

  if((ret = readArgv(argc, argv))) {
    printUsage(stderr, argv[0]);
    exit(ret);
  }

  if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
    perror("socket");
    exit(E_GENERIC);
  }

  memset((void *)&sockaddr, (int)'\0', sizeof(struct sockaddr_in));
  sockaddr.sin_family = AF_INET;
  inet_aton("0.0.0.0", (struct in_addr *)&sockaddr.sin_addr.s_addr);
  sockaddr.sin_port = htons(TCSport);
  addrlen = sizeof(sockaddr);

  if(bind(sockfd, (struct sockaddr *)&sockaddr, addrlen) == -1) {
    perror("bind");
    close(sockfd);
    exit(E_GENERIC);
  }

  trs_list = new_trs_list();


  destroy_trs_list();

  return EXIT_SUCCESS;
}



void printHelp(FILE *stream, const char *prog) {
  fprintf(stream, "RC Translation - TCS (Translation Contact Server).\n");
  printUsage(stream, prog);
  fprintf(stream,
    "Options:\n"
    "\t-h   Shows this help message and exits.\n"
    "\t-p   The TCS\' port, in the range 0-65535 (default: TCSport = %hu)\n",
  TCS_DEFAULT_PORT);
}

void printUsage(FILE *stream, const char *prog) {
  fprintf(stream, "Usage: %s [-p TCSport]\n", prog);
}

int readArgv(int argc, char **argv) {
  char c;
  int port = 0, err = EXIT_SUCCESS;

  while((c = (char)getopt(argc, argv, ":hp:")) != -1 && !err) {
    switch(c) {
      case 'h': /* Print Help & Usage */
        printHelp(stdout, argv[0]);
        exit(EXIT_SUCCESS);
      case 'p': /* Port number */ {
        long int val;
        char *endptr;

        if(port) {
          eprintf("Port was already assigned: %hu\n", TCSport);
          err = E_DUPARG;
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
            TCSport = (unsigned short)val;
            ++port;
          } /* All good! */
        }
        break;
      }
      case ':': /* Opt requires arg */
        eprintf("Option \'-%c\' requires an argument.\n", optopt);
        err = E_MISSINGARG;
        break;
      case '?': /* Unknown opt */
        eprintf("Unknown option: \'-%c\'\n", optopt);
        err = E_UNKNOWNOPT;
    }
  }

  return err;
}
