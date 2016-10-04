/**
 * RC Translation - RC@IST/UL
 * User client - RC45179 15'16
 *
 * @author: Rui Ventura (ist181045)
 * @author: Diogo Freitas (ist181586)
 * @author: Sara Azinhal (ist181700)
 */

/* Headers */
#include <arpa/inet.h>
#include <errno.h>
#include <limits.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "rctr.h" /* common header */



/* Constants */
#define PCKT_MAX_LEN 4095
#define LANG_MAX_LEN 21

#define CMD_EXIT "exit"
#define CMD_LIST "list"
#define CMD_REQ  "request"



/* Global variables */
struct hostent *TCSname; /* TCS hostent struct */
unsigned short TCSport; /* TCS' address port */



/* Prototypes */
int tcscommunicate(int sockfd, void *buf, size_t len,
  struct sockaddr *dest_addr, socklen_t *addrlen); /* Communicate with TCS */


/* Client main function */
int main(int argc, char **argv) {
  int tcsfd, ret;
  struct sockaddr_in serveraddr;
  socklen_t addrlen;

  char **languages = NULL;

  /* Setup defaults */
  TCSport = TCS_DEFAULT_PORT;
  TCSname = gethostbyname("localhost");

  /* Read and process user issued command */
  if((ret = readArgv(argc, argv)) != EXIT_SUCCESS) {
    printUsage(stderr, argv[0]);
    exit(ret);
  } //PENIS

  /* Create a socket for communication with the TCS */
  if((tcsfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
    perror("socket");
    exit(E_GENERIC);
  }

  /* Format and fill in serveraddr's fields accordingly */
  memset((void *)&serveraddr, (int)'\0', sizeof(serveraddr));
  serveraddr.sin_family      = AF_INET;
  serveraddr.sin_addr.s_addr =
    ((struct in_addr *)TCSname->h_addr_list[0])->s_addr;
  serveraddr.sin_port        = htons(TCSport);
  addrlen = sizeof(serveraddr);

  /* Main loop */
  while(1) {
    char line_buffer[PCKT_MAX_LEN]; /* Used for user input processing */
    char msg_buffer[PCKT_MAX_LEN]; /* Used for user<->server communication*/

    printf("> ");

    /* EOF read (C-d) */
    if(fgets(line_buffer, PCKT_MAX_LEN, stdin) == NULL) {
      printf(CMD_EXIT"\n");
      break;
    }

    if(!strncmp(line_buffer, CMD_EXIT"\n", sizeof(CMD_EXIT))) {
      break;
    } /* Exit program */
    else {
      char *resp; /* Server response */
      char *token; /* Auxiliary variable for tokenizing*/

      /* Parse and build messages to send to the TCS */
      if(!strncmp(line_buffer, CMD_LIST"\n", sizeof(CMD_LIST))) {
        strncpy(msg_buffer, UTCS_LANG_QUERY"\n", sizeof(UTCS_LANG_QUERY) + 1);
      } /* Build list protocol query message */
      else if(!strncmp(line_buffer, CMD_REQ" ", sizeof(CMD_REQ))) {
        if(!languages) {
          eprintf("No languages available yet. Try typing \'list\' to see a "
            "list of available languages.\n");
          continue;
        } /* 'list' hasn't been issued yet */
        else {
          int n;

          strtok(line_buffer, " "); /* 'request' not needed anymore */

          strncpy(msg_buffer, UTCS_NAMESERV_QUERY" ",
            sizeof(UTCS_NAMESERV_QUERY) + 1);

          token = strtok(NULL, " \n");
          if((n = atoi(token)) > 0 && languages[n - 1]) {

            strcat(msg_buffer, languages[n - 1]);
            strcat(msg_buffer, "\n");
          }
          else {
            eprintf("Invalid language index. Try \'list\'ing the languages "
              "available.\n"
            );
            continue;
          }

          while((token = strtok(NULL, " \n"))) {

          }
        }
      } /* Build translation request protocol query message */
      else {
        eprintf("%s: command not found or malformed\n", strtok(line_buffer, " \n"));
        continue;
      } /* Unknown command */

      /* Query TCS and receive a response */
      if(tcscommunicate(tcsfd, msg_buffer, strlen(msg_buffer),
        (struct sockaddr *)&serveraddr, &addrlen) != EXIT_SUCCESS) {
          exit(E_GENERIC);
      }

      resp = strtok(msg_buffer, " \n");
      token = strtok(NULL, " \n"); /* 2nd arg, could be error code */

      if(!strncmp(token, QUERY_INVALID, sizeof(QUERY_INVALID) - 1)) {
        eprintf("Error: No response available.\n");
      }
      else if(!strncmp(token, QUERY_BADFORM, sizeof(QUERY_BADFORM) - 1)) {
        eprintf("Error: Message corrupted.\n");
      }
      else {
        if(!strncmp(resp, UTCS_LANG_RESPONSE, sizeof(UTCS_LANG_RESPONSE) - 1)) {
          int i, n = atoi(token);

          if(languages) {
            for(i = 0; languages[i] != NULL; ++i)
              free(languages[i]);
            free(languages);
          }
          languages = (char **)malloc((n + 1) * sizeof(char *));

          for(i = 0; i < n; ++i) {
            if((token = strtok(NULL, " \n"))) {
              languages[i] = strndup(token, LANG_MAX_LEN);
              printf(" %d - %s\n", i + 1, token);
            }
          }

          languages[i] = NULL;
        }
        else if(!strncmp(resp, UTCS_NAMESERV_RESPONSE,
            sizeof(UTCS_NAMESERV_RESPONSE) - 1)) {
          int trsfd;
          struct sockaddr_in trsaddr;

          if((trsfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            perror("socket");
            exit(E_GENERIC);
          }

          memset((void *)&trsaddr, (int)'\0', sizeof(trsaddr));
          trsaddr.sin_family = AF_INET;
          inet_aton(token, (struct in_addr *)&trsaddr.sin_addr.s_addr);
          trsaddr.sin_port = htons((unsigned short)atoi(strtok(NULL, " \n")));

          if((connect(trsfd, (struct sockaddr *)&trsaddr,
              sizeof(trsaddr))) == -1) {
            int err = errno;
            if(err == ECONNREFUSED || err == ENETUNREACH || err == ETIMEDOUT) {
              eprintf("%s. Maybe try again later\n", strerror(err));
            }
          }

          /* Translation stuff */
          printf("Connected: %s:%hu\n",
            inet_ntoa((struct in_addr)trsaddr.sin_addr),
            ntohs(trsaddr.sin_port));

          close(trsfd);
        }
        else {
          eprintf("Error: Unrecognized response from server, discarded.\n");
        }
      }
    }
  }

  /* Free resources */
  if(languages) {
    int i;
    for(i = 0; languages[i] != NULL; ++i)
      free(languages[i]);
    free(languages);
  }

  /* Close socket */
  close(tcsfd);
  return EXIT_SUCCESS;
}



/* Function implementations */
int tcscommunicate(int sockfd, void *buf, size_t len,
    struct sockaddr *dest_addr, socklen_t *addrlen) {
  int err = EXIT_SUCCESS;

  if(sendto(sockfd, buf, len, 0, dest_addr, *addrlen) == -1) {
    err = errno;
    perror("sendto");
  }
  else if(recvfrom(sockfd, buf, PCKT_MAX_LEN, 0, dest_addr, addrlen) == -1) {
    err = errno;
    perror("recvfrom");
  }

  return err;
}

void printHelp(FILE *stream, const char *prog) {
  fprintf(stream, "RC Translation - User client.\n");
  printUsage(stream, prog);
  fprintf(stream,
    "Options:\n"
    "\t-h    Shows this help message and exits.\n"
    "\t-n    The TCS\' hostname, where TCSname is an IPv4 address\n"
    "\t      or a name (default: localhost)\n"
    "\t-p    The TCS\' port, in the range 0-65535 (default: TCSport = %hu)\n",
  TCS_DEFAULT_PORT);
}

void printUsage(FILE *stream, const char *prog) {
  fprintf(stream, "Usage: %s [-h] [-n TCSname] [-p TCSport]\n", prog);
}

int readArgv(int argc, char **argv) {
  char c;
  int name = 0, port = 0, err = EXIT_SUCCESS;

  while((c = (char)getopt(argc, argv, ":hn:p:")) != -1 && !err) {
    switch(c) {
      case 'h': /* Print Help & Usage */
        printHelp(stdout, argv[0]);
        exit(err);
      case 'n': /* Hostname */
        if(name) {
          eprintf("Host already assigned: %s\n", TCSname->h_addr_list[0]);
          err = E_DUPARG;
        }
        else if((TCSname = gethostbyname(optarg)) == NULL) {
          herror("gethostbyname");
          err = E_GENERIC;
        }
        else ++name;
        break;
      case 'p': /* Port number */ {
        long int val;
        char *endptr;

        if(port) {
          eprintf("Port already assigned: %hu\n", TCSport);
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
            eprintf("Invalid port: %s\n", optarg);
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
