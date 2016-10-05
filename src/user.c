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
#define LANG_MAX_LEN 21

#define CMD_EXIT "exit"
#define CMD_LIST "list"
#define CMD_REQ  "request"



/* Global variables */
struct hostent *TCSname; /* TCS hostent struct */
unsigned short TCSport; /* TCS' address port */



/* Prototypes */
int tcscommunicate(int sockfd, void *send_buf, size_t sendlen, void *recv_buf,
  size_t recv_len, struct sockaddr *dest_addr, socklen_t *addrlen);
/* Communicate with TCS */

int sendtrs(int fd, char *buffer, ssize_t size); /* Send a message to TRS */


/* Client main function */
int main(int argc, char **argv) {
  int tcsfd; /* TCS UDP socket fd */
  bool shouldRun = true; /* flag */
  struct sockaddr_in tcsaddr; /* Socket address struct for TCS */
  socklen_t addrlen;

  char **languages = NULL;

  /* Setup defaults */
  TCSport = TCS_DEFAULT_PORT;
  TCSname = gethostbyname("localhost");

  /* Read and process user issued command */
  if(readArgv(argc, argv)) {
    printUsage(stderr, argv[0]);
    exit(E_GENERIC);
  } //PENIS

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

  /* Main loop */
  while(shouldRun) {
    char line_buffer[BUF_MAX_LEN]; /* Used for user input processing */
    char msg_buffer[BUF_MAX_LEN]; /* Used for user<->server communication*/

    /* Prompt */
    printf("> ");

    /* Upon EOF (or C-d) */
    if(fgets(line_buffer, BUF_MAX_LEN, stdin) == NULL) {
      printf(CMD_EXIT"\n");
      shouldRun = false;
    }
    /* If the user doesn't type 'exit' */
    else if(strncmp(line_buffer, CMD_EXIT"\n", sizeof(CMD_EXIT))) {
      /* Auxiliary variables for tokenizing */
      char *response, *msg, *line;

      /* Parse and build messages to send to the TCS */
      if(!strncmp(line_buffer, CMD_LIST"\n", sizeof(CMD_LIST))) {
        strncpy(msg_buffer, UTCS_LANG_QUERY"\n", sizeof(UTCS_LANG_QUERY) + 1);
      } /* Build list protocol query message */
      else if(!strncmp(line_buffer, CMD_REQ" ", sizeof(CMD_REQ))) {
        if(languages) {
          int n;
          char *aux_token; /* Auxiliary token */

          strtok_r(line_buffer, " ", &line); /* 'request' not needed */
          strncpy(msg_buffer, UTCS_NAMESERV_QUERY" ",
          sizeof(UTCS_NAMESERV_QUERY) + 1);

          aux_token = strtok_r(line, " \n", &line);
          if(line) {
            switch(line[0]) {
              case 'f':
              case 't':
                if((n = atoi(aux_token)) > 0 && languages[n - 1]) {
                  sprintf(msg_buffer, "%s %s\n", msg_buffer, languages[n - 1]);
                }
                else {
                  eprintf("Invalid index. Try \'list\'ing the languages "
                    "available.\n");
                }
                break;
              default:
                eprintf("Invalid option. Available translations are \'text\' "
                  "(t) or \'file\' (f).\n");
                continue;
            }
          }
          else {
            eprintf("Missing arguments.\nUsage: "CMD_REQ" <index> "
              "<translation> [words...]\n");
          }
        } /* 'list' hasn't been issued yet */
        else {
          eprintf("No languages available yet. Try typing \'list\' to see a "
          "list of available languages.\n");
          continue;
        }
      } /* Build translation request protocol query message */
      else {
        eprintf("%s: command not found or malformed\n", strtok(line_buffer, " \n"));
        continue;
      } /* Unknown command */

      /* Query TCS and receive a response */
      if(tcscommunicate(tcsfd, msg_buffer, strlen(msg_buffer), msg_buffer,
          BUF_MAX_LEN, (struct sockaddr *)&tcsaddr, &addrlen)
          != EXIT_SUCCESS) {
        exit(E_GENERIC);
      }

      /* Get the server  */
      response = strtok_r(msg_buffer, " \n", &msg);

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
          int i, n = atoi(strtok(msg, " \n"));

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
          char * aux;
          bool isText = false;
          FILE *fp = NULL;
          long filesize = 0;
          struct sockaddr_in trsaddr;

          /* Format and setup of the sockaddr struct */
          memset((void *)&trsaddr, (int)'\0', sizeof(trsaddr));
          trsaddr.sin_family = AF_INET;
          aux = strtok(msg, " \n");
          inet_aton(aux,
            (struct in_addr *)&trsaddr.sin_addr.s_addr);
            aux = strtok(NULL, " \n");
          trsaddr.sin_port = htons((unsigned short)atoi(aux));

          /* TRS TCP socket */
          if((trsfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            perror("socket");
            exit(E_GENERIC);
          }

          /* Start building query */
          strncpy(msg_buffer, UTRS_TRANSLATE_QUERY" ",
            sizeof(UTRS_TRANSLATE_QUERY) + 1);
          isText = line[0] == 't';
          strcat(msg_buffer, strtok_r(line, " \n", &line));

          /* Distinguish between (t)ext and (f)ile */
          if(isText) {
            int i, n = 0;
            for(i = 0; line[i] != '\0'; ++i) {
              switch(line[i]) {
                case '\n':
                  while(line[++i] == ' ');
                case ' ': ++n;
              }
            }
            sprintf(msg_buffer, "%s %d %s", msg_buffer, n, line);
          }
          else if((fp = fopen((line = strtok(line, "\n")), "r"))
              != NULL) {
            if(fseek(fp, 0L, SEEK_END) != -1) {
              filesize = ftell(fp);
              rewind(fp);

              sprintf(msg_buffer, "%s %s %ld ", msg_buffer, line,
                filesize);
            }
            else {
              eprintf("fseek:");
              close(trsfd);
              continue;
            }
          }
          else {
            eprintf("Could not open file \'%s\'.\n", line);
            close(trsfd);
            continue;
          }

          sendtrs(1, msg_buffer, strlen(msg_buffer));

          if((connect(trsfd, (struct sockaddr *)&trsaddr,
              sizeof(trsaddr))) != -1) {
            printf("Connection established!\n");
          }
          else {
            int err = errno;
            if(err == ECONNREFUSED || err == ENETUNREACH || err == ETIMEDOUT) {
              eprintf("%s. Maybe try again later.\n", strerror(err));
            }
            else {
              eprintf("connect: %s\n", strerror(err));
              exit(E_GENERIC);
            }
          }

          close(trsfd);
          if(fp) fclose(fp);
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
int sendtrs(int fd, char *buffer, ssize_t size) {
  int err = EXIT_SUCCESS;
  ssize_t offset = 0, nbytes = 0;

  while(!err && size > offset) {
    if((nbytes = write(fd, buffer + offset, size - offset)) == -1) {
      err = errno;
      perror("write");
    }
    else {
      size -= nbytes;
      offset += nbytes;
    }
  }

  return err;
}

int tcscommunicate(int sockfd, void *send_buf, size_t sendlen, void *recv_buf,
    size_t recvlen, struct sockaddr *dest_addr, socklen_t *addrlen) {
  int err = EXIT_SUCCESS;

  if(sendto(sockfd, send_buf, sendlen, 0, dest_addr, *addrlen) == -1) {
    err = errno;
    perror("sendto");
  }
  else if(recvfrom(sockfd, recv_buf, recvlen, 0, dest_addr, addrlen) == -1) {
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
