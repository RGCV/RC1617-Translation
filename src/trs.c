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
#include <libgen.h>
#include <limits.h>
#include <netdb.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include "rctr.h" /* common header */



/* Constants */
/* Translation files' file names */
#define FILE_TEXT_TR "text_translation.txt"
#define FILE_FILE_TR "file_translation.txt"


/* Global variables */
volatile bool interrupted = false; /* flag to determine if SIGINT was issued */

struct hostent *TCSname; /* TCS hostent struct */
struct hostent *TRShost; /* TRS hostent struct */
unsigned short TRSport, TCSport; /* TRS and TCS ports */
char TRSlanguage[LANG_MAX_LEN]; /* The language TRS provides translation for */
char TRSname[PMSG_MAX_LEN]; /* The TRS's hostname */



/* Function prototypes */
void handle_signal(int signo); /* Signal handling function*/



/* TRS main function */
int main(int argc, char **argv) {
  int ret; /* readArgv's return value */
  int sockfd; /* TCS UDP & TCP listening socket fd */
  char send_buffer[PCKT_MAX_SIZE]; /* Buffer used to send msgs */
  char recv_buffer[PCKT_MAX_SIZE]; /* Buffer used to receive msgs */
  struct sigaction act; /* Register SIGINT handler */
  void (*old_handler)(int); /* Old SIGPIPE handler */

  FILE *filesfp, *textfp; /* translation files streams */

  struct sockaddr_in sockaddr; /* sockaddr */
  struct in_addr *addr; /* addr */
  socklen_t addrlen; /* Length of sockaddr */

  /* Setup defaults */
  TCSname = gethostbyname("localhost");
  TCSport = TCS_DEFAULT_PORT;
  TRSport = TRS_DEFAULT_PORT;

  /* Read and parse command line args */
  if((ret = readArgv(argc, argv))) {
    printUsage(stderr, argv[0]);
    exit(ret);
  }

  /* Store SIGPIPE's old handler and ignore it */
  if((old_handler = signal(SIGPIPE, SIG_IGN)) == SIG_ERR) {
    perror("signal");
  }

  /* Try to register a new signal handler for SIGINT */
  act.sa_handler = handle_signal;
  act.sa_flags = 0; /* Clean SA_RESTART set by OS, if set */
  if(sigemptyset(&act.sa_mask) == -1
      || sigaction(SIGINT, &act, NULL) == -1) {
    perror("sigaction");
    exit(E_GENERIC);
  }

  /* Try and open translation files */
  if((textfp = fopen(FILE_TEXT_TR, "r")) == NULL) {
    perror("fopen(texttr)");
    exit(E_GENERIC);
  }

  if((filesfp = fopen(FILE_FILE_TR, "r")) == NULL) {
    perror("fopen(filetr)");
    exit(E_GENERIC);
  }

  /* Get TRS hostname */
  if(gethostname(TRSname, sizeof(TRSname)) == -1) {
    perror("gethostname");
    exit(E_GENERIC);
  }

  if((TRShost = gethostbyname(TRSname)) == NULL) {
    perror("gethostbyname");
    exit(E_GENERIC);
  }

  /* Create UDP socket for TCS communication */
  if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
    perror("udp socket");
    exit(E_GENERIC);
  }

  memset((void *)&sockaddr, (int)'\0', sizeof(struct sockaddr_in));
  sockaddr.sin_family = AF_INET;
  sockaddr.sin_addr.s_addr =
    ((struct in_addr *)TCSname->h_addr_list[0])->s_addr;
  sockaddr.sin_port = htons(TCSport);
  addrlen = sizeof(sockaddr);

  /* Build message to register TRS */
  addr = (struct in_addr *)TRShost->h_addr_list[0];
  strcpy(send_buffer, SERV_TRSREG_REQ" ");
  sprintf(send_buffer, "%s %s %s %hu\n", send_buffer, TRSlanguage,
    inet_ntoa(*addr), TRSport);

  printf("[%s] Sending registry request to TCS [%s:%hu]\n", SERV_TRSREG_REQ,
    TCSname->h_name, TCSport);

  /* Send and receive the response */
  if(udp_send_recv(sockfd, send_buffer, strlen(send_buffer),
      sizeof(send_buffer) / sizeof(char), recv_buffer,
      sizeof(recv_buffer) / sizeof(char), (struct sockaddr *)&sockaddr,
      &addrlen, 5) == -1) {
    exit(E_GENERIC);
  }
  close(sockfd); /* Reopen UDP later, used for TCP hereon out until signaled */

  /* Parse response from TCS */
  if(!strncmp(recv_buffer, SERV_TRSREG_RSP" ", sizeof(SERV_TRSREG_RSP))) {
    char *token = strtok(recv_buffer, "\n"); /* Remove trailling \n */
    strtok(recv_buffer, " "); /* Response no longer needed */
    token = strtok(NULL, " "); /* Response status or error code */

    /* TCS Successfully registered TRS */
    if(!strncmp(token, SERV_STATUS_OK, sizeof(SERV_STATUS_OK) - 1)) {
      printf("[%s] Request successful! Registered to TCS\n", recv_buffer);
    }
    /* Failed to register, exit */
    else if(!strncmp(token, SERV_STATUS_NOK, sizeof(SERV_STATUS_NOK) - 1)) {
      eprintf("[%s] Request declined! Failed to register on TCS\n",
        recv_buffer);
      exit(E_GENERIC);
    }
    /* Request had syntax errors */
    else {
      eprintf("[%s] Protocol error: Request had syntax errors\n", recv_buffer);
      exit(E_GENERIC);
    }
  }
  /* Unrecognized response */
  else {
    eprintf("[%s] Protocol error: Unrecognized response\n", REQ_ERROR);
    exit(E_PROTREQERROR);
  }

  /* Create TCP socket for user communication */
  if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("tcp socket");
    exit(E_GENERIC);
  }

  memset((void *)&sockaddr, (int)'\0', sizeof(struct sockaddr_in));
  sockaddr.sin_family = AF_INET;
  sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  sockaddr.sin_port = htons(TRSport);
  addrlen = sizeof(sockaddr);

  /* Bind the TCP socket to the TRSport */
  if(bind(sockfd, (struct sockaddr *)&sockaddr, addrlen) == -1) {
    perror("bind");
    exit(E_GENERIC);
  }

  /* Listen on sockfd, with a queue of size 4 */
  if(listen(sockfd, 4) == -1) {
    perror("listen");
    exit(E_GENERIC);
  }

  /* TRS main loop */
  while(true) {
    int userfd; /* TCP user socket fd */
    char *token; /* Helpful variable for tokenizing */

    /* SIGINT received, exit cleanly(ish) */
    if(interrupted) {
      eprintf("\r[TRS] SIGINT received! Attempting to deregister\n");

      /* Build message to unregister from TCS */
      strcpy(send_buffer, SERV_TRSBYE_REQ" ");
      sprintf(send_buffer, "%s %s %s %hu\n", send_buffer, TRSlanguage,
        inet_ntoa(*addr), TRSport);

      /* Close listening socket fd */
      close(sockfd);

      /* Create socket for communication */
      if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("udp socket");
        exit(E_GENERIC);
      }

      memset((void *)&sockaddr, (int)'\0', sizeof(struct sockaddr_in));
      sockaddr.sin_family = AF_INET;
      sockaddr.sin_addr.s_addr =
        ((struct in_addr *)TCSname->h_addr_list[0])->s_addr;
      sockaddr.sin_port = htons(TCSport);
      addrlen = sizeof(sockaddr);

      /* If the communication failed (5 second delay) */
      if(udp_send_recv(sockfd, send_buffer, strlen(send_buffer),
          sizeof(send_buffer) / sizeof(char), recv_buffer,
          sizeof(recv_buffer) / sizeof(char), (struct sockaddr *)&sockaddr,
          &addrlen, 5) == -1) {
        eprintf("[TRS] Couldn\'t reach TCS, didn\'t unregister\n");
        exit(E_GENERIC);
      }

      /* Close UDP TCS socket */
      close(sockfd);

      /* Parse the response and print it */
      if(!strncmp(recv_buffer, SERV_TRSBYE_RSP" ", sizeof(SERV_TRSBYE_RSP))) {
        strtok(recv_buffer, "\n"); /* Remove trailing \n (terminator) */
        strtok(recv_buffer, " "); /* Response not needed anymore */
        token = strtok(NULL, " "); /* Status or error code */

        /* If request didn't return an error */
        if(strncmp(token, REQ_ERROR, sizeof(REQ_ERROR) - 1)) {
          printf("[%s] Request status: %s\n", SERV_TRSBYE_RSP, token);
        }
        else {
          eprintf("[%s] Protocol error: Unrecognized response\n",
            SERV_TRSBYE_RSP);
          exit(E_PROTREQERROR);
        }
      }
      else {
        eprintf("[%s] Protocol error: Unrecognized response\n", REQ_ERROR);
        exit(E_PROTREQERROR);
      }
      break;
    }
    else if((userfd = accept(sockfd, (struct sockaddr *)&sockaddr,
        &addrlen)) != -1) {
      ssize_t ret; /* Return value from rwrite/rread */
      ssize_t offset = 0; /* Bytes read offset */
      struct timeval timeout;

      printf("[%s] Accepted TCP connection from user [%s:%hu]\n",
        UTRS_TRANSLATE_REQ,
        inet_ntoa((struct in_addr)sockaddr.sin_addr),
        ntohs(sockaddr.sin_port));

      /* Setup the timeval struct */
      timeout.tv_sec = 5;
      timeout.tv_usec = 0;

      /* Set socket timeout (5 secs inactivity) */
      if(setsockopt(userfd, SOL_SOCKET, SO_SNDTIMEO, (void *)&timeout,
          sizeof(timeout)) == -1) {
        perror("setsockopt(send timeout)");
      }
      else if(setsockopt(userfd, SOL_SOCKET, SO_RCVTIMEO, (void *)&timeout,
          sizeof(timeout)) == -1) {
        perror("setsockopt(recv timeout)");
      }
      else {
        bool isText = false; /* To test if the translation is text or file */

        /* Read message from user, assuming it's a translation request */
        ret = rread(userfd, &recv_buffer[offset], sizeof(UTRS_TRANSLATE_REQ));
        if(ret == -1) continue;
        offset += ret;

        if(!strncmp(recv_buffer, UTRS_TRANSLATE_REQ" ",
            sizeof(UTRS_TRANSLATE_REQ))) {
          bool reqError = false; /* Request had syntax errors */
          char filename[FILE_MAX_LEN]; /* File to translate's file name */
          char line_buffer[LINE_MAX_LEN]; /* Line from translation file */
          size_t len = 0; /* Var to help calculate length of numbers or words */

          /* Start building response */
          strcpy(send_buffer, UTRS_TRANSLATE_RSP);

          /* Read another char, t or f or an error code */
          if(rread(userfd, &recv_buffer[offset], 1) == -1) continue;
          ++offset;

          /* Infer the request based on the first character */
          switch(recv_buffer[offset - 1]) {
            /* Read the file sent from the user */
            case 'f': {
              FILE *readfp;
              size_t bytesread, filesize;

              sprintf(send_buffer, "%s %c", send_buffer, 'f');

              /* filename start */
              if(rread(userfd, &recv_buffer[offset], 2) == -1) continue;
              offset += 2;

              while(recv_buffer[offset - 1] != ' ') {
                if((ret = rread(userfd, &recv_buffer[offset], 1)) == -1)
                  break;

                ++offset;
                ++len;
              }
              if(ret == -1) continue; /* EPIPE */

              /* The file's name */
              strncpy(filename,  &recv_buffer[offset - len - 1], len);
              if((readfp = fopen(filename, "wb")) == NULL) {
                perror("fopen");
                continue;
              }
              filename[len > FILE_MAX_LEN - 1 ? FILE_MAX_LEN - 1 : len] = '\0';

              len = 0;
              /* file size start */
              if(rread(userfd, &recv_buffer[offset], 1) == -1) {
                fclose(readfp);
                unlink(filename);
                continue;
              } /* EPIPE */
              ++offset;

              while(recv_buffer[offset - 1] != ' ') {
                if((ret = rread(userfd, &recv_buffer[offset], 1)) == -1)
                  break;

                ++offset;
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
                (size_t)strtoll(&recv_buffer[offset - len - 1], NULL, 10);

              /* Read the file */
              while(filesize > bytesread) {
                size_t sendbytes = filesize - bytesread > PCKT_MAX_SIZE
                  ? PCKT_MAX_SIZE : filesize - bytesread;

                if((offset = rread(userfd, recv_buffer, sendbytes)) == -1)
                  break;

                fwrite((void *)recv_buffer, 1, offset, readfp);
                if(ferror(readfp)) {
                  perror("ferror");
                  fclose(readfp);
                  unlink(filename);
                  break;
                }

                bytesread += offset;
              }
              if(offset == -1 || bytesread < filesize) {
                if(readfp) fclose(readfp);
                eprintf("[%s] Failed to read the entire file. "
                  "Received %zd/%zd bytes, deleting artifact\n",
                  REQ_ERROR, bytesread, filesize);

                unlink(filename);
                continue;
              } /* EPIPE */
              fclose(readfp);

              /* Terminator (cork) */
              if(rread(userfd, recv_buffer, 1) == -1) continue;

              printf("[%s] Received file translation request: \'%s\' "
                "(%zd bytes)\n",
                UTRS_TRANSLATE_REQ, filename, filesize);
              break;
            }
            /* Read the words the user sent */
            case 't': {
              int n;

              sprintf(send_buffer, "%s %c", send_buffer, 't');

              isText = true;
              /* Read a space and the beginning of the number of words */
              if(rread(userfd, &recv_buffer[offset], 2) == -1) continue;
              offset += 2;

              /* Read and count number of digits */
              while(recv_buffer[offset - 1] != ' ') {
                if((ret = rread(userfd, &recv_buffer[offset], 1)) == -1) break;

                ++offset;
                ++len;
              }
              if(ret == -1) continue; /* EPIPE */

              /* # of words */
              n = atoi(&recv_buffer[offset - len - 1]);
              sprintf(send_buffer, "%s %d", send_buffer, n);

              offset = 0;
              while(n > 0) {
                if((ret = rread(userfd, &recv_buffer[offset], 1)) == -1) break;

                ++offset;
                switch(recv_buffer[offset - 1]) {
                  case  ' ':
                  case '\n': --n;
                }
              }
              if(ret == -1) continue; /* EPIPE */

              recv_buffer[offset > PCKT_MAX_SIZE - 1
                ? PCKT_MAX_SIZE - 1 : offset] = '\0';

              printf("[%s] Received text translation request\n",
                UTRS_TRANSLATE_REQ);
              break;
            }
            /* Error code */
            default:
              sprintf(send_buffer, "%s %s\n", send_buffer, REQ_ERROR);
              eprintf("[%s] Protocol error: Request had syntax errors\n");
              rread(userfd, recv_buffer, sizeof(REQ_ERROR) - 1); /* Read rest */
              rwrite(userfd, send_buffer, strlen(send_buffer)); /* Send rsp */
              reqError = true;
          }

          if(!reqError) {
            /* Parse the rest of the data and send a response */
            if(isText) {
              char *aux_token; /* Auxiliary tokenizing var */
              strtok(recv_buffer, "\n"); /* Remove trailing \n */

              /* Word to translate */
              token = strtok_r(recv_buffer, " ", &aux_token);
              /* While there are words to translate */
              while(token && fgets(line_buffer,
                  sizeof(line_buffer) / sizeof(char), textfp) != NULL) {
                char *word  = strtok(line_buffer, " "); /* Word in file */

                if(word && !strcmp(token, word)) {
                  word = strtok(NULL, "\n");
                  printf("[%s] Translation for word \'%s\' found: %s\n",
                    UTRS_TRANSLATE_REQ, token, word);

                  /* Append word to message */
                  sprintf(send_buffer, "%s %s", send_buffer, word);

                  fseek(textfp, 0L, SEEK_SET); /* Reset cursor to start */
                  token = strtok_r(aux_token, " ", &aux_token); /* Next */
                }
              }

              /* Reached EOF, no translation available */
              if(feof(textfp)) {
                fseek(textfp, 0L, SEEK_SET); /* Reset cursor to start */
                eprintf("[%s] No translation found for word \'%s\'\n",
                  UTRS_TRANSLATE_NAVAIL, token);

                /* Build NTA message to send to user */
                sprintf(send_buffer,
                  UTRS_TRANSLATE_RSP" "UTRS_TRANSLATE_NAVAIL);
              }
              /* Successfully translated every word */
              else {
                printf("[%s] Text translation successful\n", UTRS_TRANSLATE_RSP);
              }

              strcat(send_buffer, "\n"); /* Terminator (cork) */
              /* Send of translation (or error, if NTA) */
              rwrite(userfd, send_buffer, strlen(send_buffer));
            }
            else {
              FILE *sendfp; /* File pointer to file to send */
              char *file; /* Pointer to translated file name */
              size_t filesize, bytessent = 0; /* Number of bytes sent */

              while(fgets(line_buffer, sizeof(line_buffer) / sizeof(char),
                  filesfp) != NULL) {
                file = strtok(line_buffer, " "); /* File's name in file */
                if(file && !strcmp(filename, file)) {
                  struct stat fst; /* To get file's size */

                  file = strtok(NULL, "\n"); /* Translated file name */
                  printf("[%s] Found matching file for \'%s\': "
                    "%s\n", UTRS_TRANSLATE_REQ, filename, file);
                  sprintf(send_buffer, "%s %s", send_buffer, basename(file));

                  /* Test if can open file */
                  if((sendfp = fopen(file, "rb")) == NULL) {
                    perror("fopen");
                  }
                  /* Stat file for file's size */
                  else if(stat(file, &fst) == -1) {
                    perror("stat");
                  }
                  /* Send file */
                  else {
                    size_t tosend = fst.st_size;
                    filesize = tosend;


                    sprintf(send_buffer, "%s %zd ", send_buffer, filesize);
                    if(rwrite(userfd, send_buffer, strlen(send_buffer)) == -1)
                      break;

                    while(tosend > 0) {
                      /* Determine how much to read and send thereafter */
                      size_t sendlen = (tosend >
                        sizeof(send_buffer) / sizeof(char)
                        ? sizeof(send_buffer) / sizeof(char) : tosend);
                      size_t nbytes = fread((void *)send_buffer, 1, sendlen,
                        sendfp);

                      if(ferror(sendfp)) {
                        perror("fread");
                        break;
                      }

                      if(rwrite(userfd, send_buffer, sendlen) == -1) break;
                      tosend -= nbytes;
                      bytessent += nbytes;
                    }
                  }

                  rwrite(userfd, "\n", 1);
                  break;
                }
              }

              /* Close file */
              if(sendfp) fclose(sendfp);

              /* Reached EOF, no translation available */
              if(feof(filesfp)) {
                eprintf("[%s] No translation found for file \'%s\'\n",
                  UTRS_TRANSLATE_NAVAIL, filename);

                /* Build NTA message to send to user */
                sprintf(send_buffer,
                  UTRS_TRANSLATE_RSP" "UTRS_TRANSLATE_NAVAIL"\n");
                rwrite(userfd, send_buffer, strlen(send_buffer));
              }
              /* Couldn't send the entire file */
              else if(bytessent < filesize) {
                eprintf("[TRS] Error sending file: Sent %zd/%zd bytes\n",
                  bytessent, filesize);
              }
              /* Successfully translated the entire fail */
              else {
                printf("[%s] File \'%s\' sent successfully: Sent %zd bytes\n",
                  UTRS_TRANSLATE_RSP, file, bytessent);
              }
            }
          }
        }
        /* Unrecognized request */
        else {
          eprintf("[%s] Protocol error: Unrecognized request\n");
          rwrite(userfd, REQ_ERROR"\n", sizeof(REQ_ERROR) / sizeof(char));
          continue;
        }
      }

      /* Close TCP user socket */
      close(userfd);
    }
  }

  if(filesfp) fclose(filesfp);
  if(textfp)  fclose(textfp);

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
  fprintf(stream, "RC Translation - TRS (Translation Server)\n");
  printUsage(stream, prog);
  fprintf(stream,
    "\nArguments:\n"
    "  language   The language the TRS provides translations from\n"

    "\nOptions:\n"
    "\t-h   Shows this help message and exits\n"
    "\t-n   The TCS\' hostname, where TCSname is an IPv4 address\n"
    "\t     or a name (default: localhost)\n"
    "\t-p   The TRS\' port, in the range %1$hu-65535 "
    "(default: TRSport = %2$hu)\n"
    "\t-e   The TCS\'s port, in the range %1$hu-65535 "
    "(default: TCSport = %3$hu)\n",
  PORT_MIN, TRS_DEFAULT_PORT, TCS_DEFAULT_PORT);
}

void printUsage(FILE *stream, const char *prog) {
  fprintf(stream, "Usage: %s <language> [-p TRSport] [-n TCSname] "
    "[-e TCSport]\n", prog);
}

int readArgv(int argc, char **argv) {
  char c;
  int i, err = EXIT_SUCCESS;
  bool TRSlangflg = false,
    TRSportflg = false,
    TCSnameflg = false,
    TCSportflg = false;

  /* Handle options */
  while((c = (char)getopt(argc, argv, ":hp:n:e:")) != -1) {
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
          else if(val < PORT_MIN || val > USHRT_MAX || endptr == optarg
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

  /* Other arguments (looking for language) */
  for(i = optind; i < argc; ++i) {
    if(!TRSlangflg) {
      TRSlangflg = true;
      strncpy(TRSlanguage, argv[i], LANG_MAX_LEN - 1);
      TRSlanguage[LANG_MAX_LEN - 1] = '\0';
    }
  }

  /* If a language wasn't passed */
  if(!err && !TRSlangflg) {
    eprintf("%s: Missing language argument\n", argv[0]);
    err = E_MISSINGARG;
  }

  return err;
}
