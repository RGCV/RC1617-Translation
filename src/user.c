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
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include "rctr.h" /* common header */



/* Constants */
#define BUF_MAX_LEN 4095

#define CMD_EXIT "exit"
#define CMD_LIST "list"
#define CMD_REQ  "request"



/* Global variables */
struct hostent *TCSname; /* TCS hostent struct */
unsigned short TCSport; /* TCS' address port */



/* Client main function */
int main(int argc, char **argv) {
  int ret; /* readArgv return code */
  int tcsfd; /* TCS UDP socket fd */
  bool shouldRun = true; /* should the loop be running flag */
  struct sockaddr_in tcsaddr; /* Socket address struct for TCS */
  socklen_t addrlen; /* length of the sockaddr */

  char **languages = NULL; /* Available languages array */

  /* Setup defaults */
  TCSport = TCS_DEFAULT_PORT;
  TCSname = gethostbyname("localhost");

  /* Read and process user issued command */
  if((ret = readArgv(argc, argv))) {
    printUsage(stderr, argv[0]);
    exit(ret);
  }

  /* Create a socket for communication with the TCS */
  if((tcsfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
    perror("socket");
    exit(E_GENERIC);
  }

  /* Format and fill in tcsaddr's fields accordingly */
  memset((void *)&tcsaddr, (int)'\0', sizeof(tcsaddr));
  tcsaddr.sin_family      = AF_INET;
  tcsaddr.sin_addr.s_addr =
    ((struct in_addr *)TCSname->h_addr_list[0])->s_addr;
  tcsaddr.sin_port        = htons(TCSport);
  addrlen = sizeof(tcsaddr);

  /* Main loop */
  while(shouldRun) {
    char *delim = " \n\t"; /* Potential word delimiters */
    char line_buffer[BUF_MAX_LEN]; /* Used for user input processing */

    /* Prompt */
    printf("> ");

    /* Upon EOF (or C-d) */
    if(fgets(line_buffer, BUF_MAX_LEN, stdin) == NULL) {
      printf(CMD_EXIT"\n");
      shouldRun = false;
    }
    /* Main program: User typed a command different than exit */
    else if(strncmp(line_buffer, CMD_EXIT"\n", sizeof(CMD_EXIT))) {
      bool isText = false;
      char *filename = NULL;

      char *response, *msg, *line; /* Auxiliary variables for tokenizing */
      char tcs_buffer[PCKT_SIZE_MAX]; /* Used for user<->tcs communication*/
      char trs_buffer[PCKT_SIZE_MAX]; /* Used for user<->trs communication */

      /* Parse and build messages to send to the TCS and TRS, if needed */
      if(!strncmp(line_buffer, CMD_LIST"\n", sizeof(CMD_LIST))) {
        strcpy(tcs_buffer, UTCS_LANG_QUERY"\n");
      } /* Build list protocol query message */
      else if(!strncmp(line_buffer, CMD_REQ" ", sizeof(CMD_REQ))) {
        /* Check if user already requested the list of languages */
        if(languages) {
          int n;
          char aux_buffer[BUF_MAX_LEN];
          char *aux_token;

          strcpy(tcs_buffer, UTCS_NAMESERV_QUERY);

          strtok_r(line_buffer, delim, &line); /* 'request' not needed */
          if((n = atoi(strtok_r(line, delim, &line))) > 0 && languages[n - 1]) {
            if(line) {
              switch(line[0]) {
                case 't':
                  isText = true;
                case 'f':
                  sprintf(tcs_buffer, "%s %s\n", tcs_buffer, languages[n - 1]);
                  break;
                default:
                  eprintf("Invalid option. Available translations are \'text\' "
                    "(t) or \'file\' (f).\n");
                  continue;
              }
            }
            else {
              eprintf("Missing translation type argument (t or f).\n");
              continue;
            }
          }
          else {
            eprintf("Invalid language index.\n");
            continue;
          }

          n = 0;
          strcpy(trs_buffer, UTRS_TRANSLATE_QUERY);
          sprintf(trs_buffer, "%s %s", trs_buffer,
            strtok_r(line, delim, &line)); /* t or f */

          /* Save the filename for later */
          if(isText) {
            /* Let's count some words */
            strcpy(aux_buffer, line);
            aux_token = strtok(aux_buffer, delim);
            while(aux_token && strlen(aux_token) > 0) {
              ++n;
              aux_token = strtok(NULL, delim);
            }

            /* More than one 'valid' token was found */
            if(n > 0) {
              sprintf(trs_buffer, "%s %d", trs_buffer, n);

              aux_token = strtok(line, delim);
              while(aux_token) {
                sprintf(trs_buffer, "%s %s", trs_buffer, aux_token);

                aux_token = strtok(NULL, delim);
              }
            }
            else {
              eprintf("%s: command malformed.\n", line_buffer);
              continue;
            }
          }
          else {
            /* File information struct */
            struct stat fst;

            filename = strtok(line, delim);
            if(stat(filename, &fst) == -1) {
              perror("stat");
              exit(E_GENERIC);
            }

            sprintf(trs_buffer, "%s %s %ld ", trs_buffer, filename,
              fst.st_size);
          }
        }
        else {
          eprintf("No languages available yet. Try typing \'list\' to see a "
            "list of available languages.\n");
          continue;
        }
      } /* Build translation request protocol query messages */
      else {
        eprintf("%s: command not found or malformed.\n",
          strtok(line_buffer, " \n"));
        continue;
      } /* Unknown command */

      /* Query TCS and receive a response */
      if(udp_send_recv(tcsfd, tcs_buffer, strlen(tcs_buffer), PCKT_SIZE_MAX,
          (struct sockaddr *)&tcsaddr, &addrlen, 5) != EXIT_SUCCESS) {
        exit(E_GENERIC);
      }

      /* Get the server  */
      response = strtok_r(tcs_buffer, delim, &msg);

      /* Check if token matches an error code */
      if(!strncmp(msg, QUERY_INVALID, sizeof(QUERY_INVALID) - 1)) {
        eprintf("Protocol error: No response available.\n");
        exit(E_PROTINVALID);
      }
      else if(!strncmp(msg, QUERY_BADFORM, sizeof(QUERY_BADFORM) - 1)) {
        eprintf("Protocol error: Query had a bad form.\n");
        exit(E_PROTQBADFORM);
      }
      else {
        if(!strncmp(response, UTCS_LANG_RESPONSE,
            sizeof(UTCS_LANG_RESPONSE) - 1)) {
          int i, n = atoi(strtok(msg, delim));

          if(languages) {
            for(i = 0; languages[i] != NULL; ++i)
              free(languages[i]);
            free(languages);
          }
          languages = (char **)malloc((n + 1) * sizeof(char *));

          for(i = 0; i < n; ++i) {
            if((msg = strtok(NULL, " \n"))) {
              languages[i] = strndup(msg, LANG_MAX_LEN - 1);
              printf(" %d - %s\n", i + 1, msg);
            }
          }

          languages[i] = NULL;
        }
        else if(!strncmp(response, UTCS_NAMESERV_RESPONSE,
            sizeof(UTCS_NAMESERV_RESPONSE) - 1)) {
          int trsfd;
          struct sockaddr_in trsaddr;

          /* Format and setup of the sockaddr struct */
          memset((void *)&trsaddr, (int)'\0', sizeof(trsaddr));
          trsaddr.sin_family = AF_INET;
          inet_aton(strtok(msg, delim),
            (struct in_addr *)&trsaddr.sin_addr.s_addr);
          trsaddr.sin_port = htons((unsigned short)atoi(strtok(NULL, delim)));

          /* TRS TCP socket */
          if((trsfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            perror("socket");
            exit(E_GENERIC);
          }

          if((connect(trsfd, (struct sockaddr *)&trsaddr,
              sizeof(trsaddr))) != -1) {
            rwrite(trsfd, trs_buffer, strlen(trs_buffer));
            if(!isText) {
              FILE *sendfp;
              struct stat fst;

              if((sendfp = fopen(filename, "r")) == NULL) {
                perror("fopen");
              }
              else if(stat(filename, &fst) == -1) {
                perror("stat");
              }
              else {
                size_t filesize = fst.st_size;

                while(filesize > 0) {
                  size_t sendlen = (filesize > PCKT_SIZE_MAX
                    ? PCKT_SIZE_MAX : filesize);
                  size_t nbytes = fread((void *)trs_buffer, 1, sendlen, sendfp);

                  if(ferror(sendfp)) {
                    perror("fread");

                    close(trsfd);
                    fclose(sendfp);
                    exit(E_GENERIC);
                  }

                  rwrite(trsfd, trs_buffer, sendlen);
                  filesize -= nbytes;
                }

                if(sendfp) fclose(sendfp);
              }
            }

            /* Terminator (cork) */
            rwrite(trsfd, "\n", 1);

            /* Read the protocol message code */
            rread(trsfd, trs_buffer, sizeof(UTRS_TRANSLATE_RESPONSE));
            if(!strncmp(trs_buffer, UTRS_TRANSLATE_RESPONSE" ",
                sizeof(UTRS_TRANSLATE_RESPONSE))) {
              /*t or f and a space or 1st 2 letters of error code */
              rread(tsrfd, trs_buffer, 2);

              switch(trs_buffer[0]) {
                case 'f': {
                  /* TODO: Read file */
                  break;
                }
                case 't': {
                  /* TODO: Read words */
                  break;
                }
                default:
                  if(trs_buffer[0] == UTRS_TRANSLATE_NOTAVAIL[0]) {
                    eprintf("No translation available.\n");
                    rread(trsfd, trs_buffer,
                      sizeof(UTRS_TRANSLATE_NOTAVAIL) - 2);
                  }
                  else if(trs_buffer[0] == QUERY_BADFORM[0]) {
                    eprintf("Protocol error: Query had a bad form.\n");
                    rread(trsfd, trs_buffer, sizeof(QUERY_BADFORM) - 2);
                    close(trsfd);
                    exit(E_GENERIC);
                  }
                  break;
              }
            }
            else {
              eprintf("Protocol error: Unrecognized response.\n");

              close(trsfd);
              exit(E_GENERIC);
            }
          }
          else {
            int err = errno;
            if(err == ECONNREFUSED || err == ENETUNREACH || err == ETIMEDOUT) {
              eprintf("%s. Maybe try again later.\n", strerror(err));
            }
            else {
              eprintf("connect: %s\n", strerror(err));
              close(trsfd);
              exit(E_GENERIC);
            }
          }

          close(trsfd);
        }
        else {
          eprintf("Error: Unrecognized response from server, discarded.\n");
          exit(E_GENERIC);
        }
      }
    }
    /* Otherwise (exit command issued) */
    else shouldRun = false;
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
  int err = EXIT_SUCCESS;
  bool name = false, port = false;

  while((c = (char)getopt(argc, argv, ":hn:p:")) != -1 && !err) {
    switch(c) {
      case 'h': /* Print Help & Usage */
        printHelp(stdout, argv[0]);
        exit(err);
      case 'n': /* Hostname */
        if(!name) {
          if((TCSname = gethostbyname(optarg)) == NULL) {
            herror("gethostbyname");
            err = E_GENERIC;
          }
          else name = true;
        }
        else {
          eprintf("Host already assigned: %s\n", TCSname->h_addr_list[0]);
          err = E_DUPOPT;
        }
        break;
      case 'p': /* Port number */ {
        long int val;
        char *endptr;

        if(!port) {
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
          }
          else {
            TCSport = (unsigned short)val;
            port = true;
          } /* All good! */
        }
        else {
          eprintf("Port already assigned: %hu\n", TCSport);
          err = E_DUPOPT;
        } /* Out of port (0-65535) range, blank or has alpha characters */
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
