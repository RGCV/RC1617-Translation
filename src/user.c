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
#include <libgen.h>
#include <limits.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include "rctr.h" /* common header */



/* Constants */
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

  memset((void *)&tcsaddr, (int)'\0', sizeof(tcsaddr));
  tcsaddr.sin_family      = AF_INET;
  tcsaddr.sin_addr.s_addr =
    ((struct in_addr *)TCSname->h_addr_list[0])->s_addr;
  tcsaddr.sin_port        = htons(TCSport);
  addrlen = sizeof(tcsaddr);

  /* Main loop */
  while(shouldRun) {
    char line_buffer[LINE_MAX_LEN]; /* Used for user input processing */

    /* Prompt */
    printf("> ");

    /* Upon EOF (or C-d) */
    if(fgets(line_buffer, LINE_MAX_LEN, stdin) == NULL) {
      printf(CMD_EXIT"\n");
      shouldRun = false;
    }
    /* Main program: User typed a command different than exit */
    else if(strncmp(line_buffer, CMD_EXIT"\n", sizeof(CMD_EXIT))) {
      bool isText = false; /* To distinguish translation query types */
      char filename[FILE_MAX_LEN]; /* If needed, will keep a file's name */

      char *token; /* Auxiliary variables for tokenizing */
      char tcs_buffer[PCKT_MAX_SIZE]; /* Used for user<->tcs communication */
      char trs_buffer[PCKT_MAX_SIZE]; /* Used for user<->trs communication */

      /* Build list protocol query message */
      if(!strncmp(line_buffer, CMD_LIST"\n", sizeof(CMD_LIST))) {
        strcpy(tcs_buffer, UTCS_LANG_QUERY"\n");
      }
      /* Build translation request protocol query messages */
      else if(!strncmp(line_buffer, CMD_REQ" ", sizeof(CMD_REQ))) {
        /* Check if user already requested the list of languages */
        if(languages) {
          int n;

          /* Start setting up communication buffers */
          strcpy(tcs_buffer, UTCS_NAMESERV_QUERY);
          strcpy(trs_buffer, UTRS_TRANSLATE_QUERY);

          strtok(line_buffer, "\n"); /* Remove trailing \n */

          strtok_r(line_buffer, " ", &token); /* 'request' not needed */
          if((n = atoi(strtok_r(token, " ", &token))) > 0 && languages[n - 1]) {
            if(token) {
              switch(token[0]) {
                case 't':
                  isText = true;
                case 'f':
                  sprintf(tcs_buffer, "%s %s\n", tcs_buffer, languages[n - 1]);
                  break;
                default:
                  eprintf("Invalid option. Available translations are \'text\' "
                    "(t) or \'file\' (f)\n");
                  continue;
              }
            }
            /* Missing arg for translation type */
            else {
              eprintf("Missing translation type argument (t or f)\n");
              continue;
            }
          }
          /* n < 0 or languages[n - 1] doesn't exist */
          else {
            eprintf("Invalid language index\n");
            continue;
          }

          n = 0;
          sprintf(trs_buffer, "%s %s", trs_buffer,
            strtok_r(token, " ", &token)); /* t or f */

          /* Build message for text translation */
          if(isText) {
            char buffer[LINE_MAX_LEN];

            /* Let's count some words */
            strcpy(buffer, token);
            token = strtok(token, " ");
            while(token) {
              ++n;
              token = strtok(NULL, " ");
            }

            /* More than one 'valid' token was found */
            if(n > 0) {
              sprintf(trs_buffer, "%s %d", trs_buffer, n);

              token = strtok(buffer, " ");
              while(token) {
                sprintf(trs_buffer, "%s %s", trs_buffer, token);
                token = strtok(NULL, " ");
              }
            }
            /* Didn't find a valid token */
            else {
              eprintf("%s: command malformed\n", line_buffer);
              continue;
            }
          }
          /* Build for file otherwise */
          else {
            /* File information struct */
            struct stat fst;

            strncpy(filename, strtok(token, " "), FILE_MAX_LEN - 1);
            filename[FILE_MAX_LEN] = '\0'; /* Good measure */
            if(stat(filename, &fst) == -1) {
              perror("stat");
              continue;
            }

            sprintf(trs_buffer, "%s %s %ld ", trs_buffer, basename(filename),
              fst.st_size);
          }
        }
        else {
          eprintf("No languages available yet. Try typing \'list\' to see a "
            "list of available languages\n");
          continue;
        }
      }
      /* Unknown command */
      else {
        eprintf("%s: command not found or malformed\n",
          strtok(line_buffer, " \n"));
        continue;
      }

      /* Query TCS and receive a response (5 second timeout) */
      if(udp_send_recv(tcsfd, tcs_buffer, strlen(tcs_buffer),
          sizeof(tcs_buffer) / sizeof(char), (struct sockaddr *)&tcsaddr,
          &addrlen, 5) != EXIT_SUCCESS) {
        exit(E_GENERIC);
      }

      /* Get the server's response */
      strtok(tcs_buffer, "\n"); /* Remove trailing \n (terminator) */
      strtok_r(tcs_buffer, " ", &token); /* Point to the rest of the args */

      /* Check if token matches an error code */
      if(!strncmp(token, QUERY_INVALID, sizeof(QUERY_INVALID) - 1)) {
        eprintf("Protocol error: No response available\n");
        exit(E_PROTINVALID);
      }
      else if(!strncmp(token, QUERY_BADFORM, sizeof(QUERY_BADFORM) - 1)) {
        eprintf("Protocol error: Query had a bad form\n");
        exit(E_PROTQBADFORM);
      }
      else {
        if(!strncmp(tcs_buffer, UTCS_LANG_RESPONSE,
            sizeof(UTCS_LANG_RESPONSE) - 1)) {
          int i, n = atoi(strtok(token, " "));

          if(languages) {
            for(i = 0; languages[i] != NULL; ++i)
              free(languages[i]);
            free(languages);
          }
          languages = (char **)malloc((n + 1) * sizeof(char *));

          for(i = 0; i < n; ++i) {
            if((token = strtok(NULL, " "))) {
              languages[i] = strndup(token, LANG_MAX_LEN - 1);
              printf(" %d - %s\n", i + 1, token);
            }
          }

          languages[i] = NULL;
        }
        else if(!strncmp(tcs_buffer, UTCS_NAMESERV_RESPONSE,
            sizeof(UTCS_NAMESERV_RESPONSE) - 1)) {
          int trsfd;
          struct sockaddr_in trsaddr;
          void (*old_handler)(int); /* old SIGPIPE sighandler */

          /* If SIGPIPE can't be ignored */
          if((old_handler = signal(SIGPIPE, SIG_IGN)) == SIG_ERR) {
            perror("Failed to ignore SIGPIPE");
            exit(E_GENERIC);
          }

          /* Setup TRS TCP socket */
          if((trsfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            perror("socket");
            exit(E_GENERIC);
          }

          memset((void *)&trsaddr, (int)'\0', sizeof(trsaddr));
          trsaddr.sin_family = AF_INET;
          inet_aton(strtok(token, " "),
            (struct in_addr *)&trsaddr.sin_addr.s_addr);
          trsaddr.sin_port = htons((unsigned short)atoi(strtok(NULL, " ")));

          /* TCP communication with the TRS */
          if((connect(trsfd, (struct sockaddr *)&trsaddr,
              sizeof(trsaddr))) != -1) {
            ssize_t ret; /* Return from rwrite or rread */
            ssize_t offset = 0; /* bytes read offset */

            /* Send initial message (full, if text translation) */
            ret = rwrite(trsfd, trs_buffer, strlen(trs_buffer));
            if(ret == -1) continue; /* EPIPE */

            /* Send file */
            if(!isText) {
              FILE *sendfp;
              struct stat fst;

              if((sendfp = fopen(filename, "rb")) == NULL) {
                perror("fopen");
                exit(E_GENERIC);
              }
              else if(stat(filename, &fst) == -1) {
                perror("stat");
                exit(E_GENERIC);
              }
              else {
                size_t filesize = fst.st_size;

                while(filesize > 0) {
                  size_t sendlen = (filesize > PCKT_MAX_SIZE
                    ? PCKT_MAX_SIZE : filesize);
                  size_t nbytes = fread((void *)trs_buffer, 1, sendlen, sendfp);

                  if(ferror(sendfp)) {
                    perror("fread");
                    exit(E_GENERIC);
                  }

                  ret = rwrite(trsfd, trs_buffer, sendlen);
                  if(ret == -1) break;
                  filesize -= nbytes;
                }

                fclose(sendfp);
              }
            }
            if(ret == -1) continue; /* EPIPE */

            /* Terminator (cork) */
            ret = rwrite(trsfd, "\n", 1);
            if(ret == -1) continue; /* EPIPE */

            /* Read the protocol message code */
            ret = rread(trsfd, &trs_buffer[offset],
              sizeof(UTRS_TRANSLATE_RESPONSE));
            if(ret == -1) continue; /* EPIPE */

            if(!strncmp(trs_buffer, UTRS_TRANSLATE_RESPONSE" ",
                sizeof(UTRS_TRANSLATE_RESPONSE))) {
              char c;
              size_t len = 0; /* for parsing filesize or # of words */

              /*t or f or 1st letter of error code */
              ret = rread(trsfd, &trs_buffer[offset], 1);
              if(ret == -1) continue;
              offset += ret;

              switch(c = trs_buffer[offset - 1]) {
                /* File translation */
                case 'f': {
                  FILE *readfp;
                  size_t bytesread, filesize;

                  /* filename start */
                  ret = rread(trsfd, &trs_buffer[offset], 2);
                  if(ret == -1) continue; /* EPIPE */
                  offset += ret;

                  while(trs_buffer[offset - 1] != ' ') {
                    ret = rread(trsfd, &trs_buffer[offset], 1);
                    if(ret == -1) break; /* EPIPE */

                    offset += ret;
                    ++len;
                  }
                  if(ret == -1) continue; /* EPIPE */

                  /* The file's name */
                  strncpy(filename,  &trs_buffer[offset - len - 1], len);
                  if((readfp = fopen(filename, "wb")) == NULL) {
                    perror("fopen");
                    continue;
                  }

                  len = 0;
                  /* file size start */
                  ret = rread(trsfd, &trs_buffer[offset], 1);
                  if(ret == -1) {
                    fclose(readfp);
                    unlink(filename);
                    continue;
                  } /* EPIPE */

                  offset += ret;
                  while(trs_buffer[offset - 1] != ' ') {
                    ret = rread(trsfd, &trs_buffer[offset], 1);
                    if(ret == -1) break; /* EPIPE */

                    offset += ret;
                    ++len;
                  }
                  if(ret == -1) {
                    fclose(readfp);
                    unlink(filename);
                    continue;
                  } /* EPIPE */

                  /* The file's size */
                  bytesread = 0;
                  filesize =
                    (size_t)strtoll(&trs_buffer[offset - len - 1], NULL, 10);

                  /* Read the file */
                  while(filesize > bytesread) {
                    offset = rread(trsfd, trs_buffer,
                      (filesize - bytesread) > PCKT_MAX_SIZE
                      ? PCKT_MAX_SIZE : filesize - bytesread);
                    if(offset == -1) break; /* EPIPE */

                    fwrite((void *)trs_buffer, 1, offset, readfp);
                    if(ferror(readfp)) {
                      perror("ferror");
                      fclose(readfp);
                      close(trsfd);
                      unlink(filename);
                      exit(E_GENERIC);
                    }

                    bytesread += offset;
                  }
                  if(offset == -1) {
                    fclose(readfp);
                    unlink(filename);
                    continue;
                  } /* EPIPE */
                  fclose(readfp);

                  /* Terminator (cork) */
                  ret = rread(trsfd, trs_buffer, 1);
                  if(ret == -1) continue; /* EPIPE */

                  printf("Translation successful: \'%s\' (%lu bytes)\n",
                    filename, bytesread);
                  break;
                }
                /* Text translation */
                case 't': {
                  int n;

                  /* Read a space and the beginning of the number of words */
                  ret = rread(trsfd, &trs_buffer[offset], 2); /* ' #' */
                  if(ret == -1) continue; /* EPIPE */
                  offset += ret;

                  /* Read and count number of digits */
                  while(trs_buffer[offset - 1] != ' ') {
                    ret = rread(trsfd, &trs_buffer[offset], 1);
                    if(ret == -1) break; /* EPIPE */

                    offset += ret;
                    ++len;
                  }
                  if(ret == -1) continue; /* EPIPE */

                  /* # of words */
                  n = atoi(&trs_buffer[offset - len - 1]);

                  offset = 0;
                  while(n > 0) {
                    ret = rread(trsfd, &trs_buffer[offset], 1);
                    if(ret == -1) break; /* EPIPE */

                    offset += ret;
                    switch(trs_buffer[offset - 1]) {
                      case  ' ':
                      case '\n': --n;
                    }
                  }
                  if(ret == -1) continue; /* EPIPE */

                  trs_buffer[offset] = '\0';
                  printf("Translation successful: %s", trs_buffer);
                  break;
                }
                /* Error code */
                default:
                  if(c == UTRS_TRANSLATE_NOTAVAIL[0]) {
                    eprintf("No translation available\n");
                    rread(trsfd, trs_buffer,
                      sizeof(UTRS_TRANSLATE_NOTAVAIL) - 1);
                  }
                  else if(c == QUERY_BADFORM[0]) {
                    eprintf("Protocol error: Query had a bad form\n");
                    rread(trsfd, trs_buffer, sizeof(QUERY_BADFORM) - 1);
                    exit(E_GENERIC);
                  }
                  break;
              }
            }
            else {
              eprintf("Protocol error: Unrecognized response\n");
              exit(E_GENERIC);
            }
          }
          /* Connection to trs server failed */
          else {
            int err = errno;

            /* If the connection was refused,  .. prompt to try later */
            if(err == ECONNREFUSED || err == ENETUNREACH || err == ETIMEDOUT) {
              eprintf("%s. Maybe try again later\n", strerror(err));
            }
            /* If not, just print error message and exit */
            else {
              eprintf("connect: %s\n", strerror(err));
              exit(E_GENERIC);
            }
          }

          close(trsfd);

          if(signal(SIGPIPE, old_handler) == SIG_ERR) {
            perror("Failed to restore SIGPIPE handler");
            exit(E_GENERIC);
          }
        }
        else {
          eprintf("Error: Unrecognized response from server discarded\n");
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
  fprintf(stream, "RC Translation - User client\n");
  printUsage(stream, prog);
  fprintf(stream,
    "Options:\n"
    "\t-h    Shows this help message and exits\n"
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
        eprintf("Option \'-%c\' requires an argument\n", optopt);
        err = E_MISSINGARG;
        break;
      case '?': /* Unknown opt */
        eprintf("Unknown option: \'-%c\'\n", optopt);
        err = E_UNKNOWNOPT;
    }
  }

  return err;
}
