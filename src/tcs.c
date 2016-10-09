/**
 * RC Translation
 * TCS (Translation Central Server) - RC45179 15'16
 *
 * @author: Rui Ventura (ist181045)
 * @author: Diogo Freitas (ist181586)
 * @author: Sara Azinhal (ist181700)
 */

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
#include <unistd.h>

#include "rctr.h"
#include "trs_list.h"



/* Global variables */
unsigned short TCSport; /* Port to bind TCS to */



/* Client main function */
int main(int argc, char **argv) {
  int ret; /* readArgv return code */
  int sockfd; /* TCS UDP socket fd */
  bool shouldRun = true; /* should the loop be running flag */
  struct sockaddr_in sockaddr; /* sockaddr */
  socklen_t addrlen; /* Length of sockaddr */

  trs_list_t *trs_list; /* List of trs registries (languages available) */

  /* Setup defaults */
  TCSport = TCS_DEFAULT_PORT;

  /* Read and parse command line args */
  if((ret = readArgv(argc, argv))) {
    printUsage(stderr, argv[0]);
    exit(ret);
  }

  /* Create socket for communication */
  if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
    perror("socket");
    exit(E_GENERIC);
  }

  memset((void *)&sockaddr, (int)'\0', sizeof(struct sockaddr_in));
  sockaddr.sin_family = AF_INET;
  inet_aton("0.0.0.0", (struct in_addr *)&sockaddr.sin_addr.s_addr);
  sockaddr.sin_port = htons(TCSport);
  addrlen = sizeof(sockaddr);

  /* Bind the socket to TCSport */
  if(bind(sockfd, (struct sockaddr *)&sockaddr, addrlen) == -1) {
    perror("bind");
    close(sockfd);
    exit(E_GENERIC);
  }

  /* Init trs_list */
  trs_list = new_trs_list();

  while(shouldRun) {
    char send_buffer[PCKT_SIZE_MAX]; /* Buffer used to send msgs */
    char recv_buffer[PCKT_SIZE_MAX]; /* Buffer used to receive msgs */

    /* Wait for message */
    if(recvfrom(sockfd, (void *)recv_buffer, PCKT_SIZE_MAX, 0,
        (struct sockaddr *)&sockaddr, &addrlen) == -1) {
      perror("recvfrom");
      close(sockfd);
      destroy_trs_list(trs_list);
      exit(E_GENERIC);
    }
    else {
      char c = recv_buffer[0]; /* First char to infer query and sender */

      /* Query from user */
      if(c == UTCS_LANG_QUERY[0] || c == UTCS_NAMESERV_QUERY[0]) {
        /* Languages query */
        if(!strncmp(recv_buffer, UTCS_LANG_QUERY"\n",
            sizeof(UTCS_LANG_QUERY))) {
          trs_entry_t *node = trs_list->head;

          strtok(recv_buffer, "\n"); /* Remove trailing \n (terminator) */
          strtok(NULL, " "); /* Next strtok should return NULL, if correct */

          printf("[%s] Received languages query from user [%s:%hu]\n",
            UTCS_LANG_QUERY, inet_ntoa(sockaddr.sin_addr),
            ntohs(sockaddr.sin_port));

          strcpy(send_buffer, UTCS_LANG_RESPONSE);

          /* No garbage at the end means query was well formed */
          if(strtok(NULL, delim) == NULL) {
            /* Check languages exist */
            if(trs_list->size > 0) {
              sprintf(send_buffer, "%s %d", send_buffer, (int)trs_list->size);

              while(node) {
                sprintf(send_buffer, "%s %s", send_buffer, node->language);
                node = node->next;
              }

              printf("[%s] Request Successful! List of languages sent!\n",
                UTCS_LANG_RESPONSE);
            }
            /* Language not found */
            else {
              eprintf("[%s] Protocol error: Language not found\n",
                QUERY_INVALID);
              sprintf(send_buffer, "%s %s", send_buffer, QUERY_INVALID);
            }
          }
          /* Query had syntax errors*/
          else {
            eprintf("[%s] Protocol error: Query had a bad form\n",
              QUERY_BADFORM);
            sprintf(send_buffer, "%s %s", send_buffer, QUERY_BADFORM);
          }
        }
        /* TRS query */
        else if(!strncmp(recv_buffer, UTCS_NAMESERV_QUERY" ",
            sizeof(UTCS_NAMESERV_QUERY))) {
          char *token; /* Help with tokenizing */
          trs_entry_t *node = NULL;
          strtok(recv_buffer, "\n"); /* Removing trailing \n (terminator) */

          strtok(recv_buffer, " "); /* Query type not needed anymore */
          token = strtok(NULL, " "); /* Language, supposedly */

          printf("[%s] Received translation server query from user [%s:%hu]\n",
            UTCS_NAMESERV_QUERY, inet_ntoa(sockaddr.sin_addr),
            ntohs(sockaddr.sin_port));

          strcpy(send_buffer, UTCS_NAMESERV_RESPONSE);
          /* And there's nothing else to read (msg has good form) */
          if(strtok(NULL, " ") == NULL && token) {
            /* If the server was found */
            if((node = get_trs_entry_lang(trs_list, token))) {
              sprintf(send_buffer, "%s %s %hu", send_buffer,
                node->address, node->port);
            }
            /* TRS for the queried language wasn't found */
            else {
              eprintf("[%s] Protocol error: No TRS for language \'%s\' found\n",
                QUERY_INVALID, token);
              sprintf(send_buffer, "%s %s", send_buffer, QUERY_INVALID);
            }
          }
          /* Query had syntax errors */
          else {
              eprintf("[%s] Protocol error: Query had a bad form\n",
                QUERY_BADFORM);
              sprintf(send_buffer, "%s %s", send_buffer, QUERY_BADFORM);
            }
        }
        /* Protocol message unrecognized */
        else {
          eprintf("[%s] Protocol error: Unrecognized query\n", QUERY_BADFORM);
          strcpy(send_buffer, QUERY_BADFORM); /* ERR */
        }
      }
      else if(c == SERV_TRSREG_QUERY[0] || c == SERV_TRSBYE_QUERY[0]) {
        /* Bools for flagging registry vs. removal and query form */
        bool regEntry = false, badform = false;
        /* For holding the info of the TRS to be registered or removed */
        char *address, *language;
        unsigned short port;

        /* TRS Registry */
        if(!strncmp(recv_buffer, SERV_TRSREG_QUERY,
            sizeof(SERV_TRSREG_QUERY) - 1)) {
          printf("[%s] Received TRS server registry query from %s:%hu\n",
            SERV_TRSREG_QUERY, inet_ntoa(sockaddr.sin_addr),
            ntohs(sockaddr.sin_port));

          regEntry = true;
          strcpy(send_buffer, SERV_TRSREG_RESPONSE);
        }
        /* TRS Removal */
        else if(!strncmp(recv_buffer, SERV_TRSBYE_QUERY,
            sizeof(SERV_TRSBYE_QUERY) - 1)) {
          printf("[%s] Received TRS server unregistry query from %s:%hu\n",
            SERV_TRSBYE_QUERY, inet_ntoa(sockaddr.sin_addr),
            ntohs(sockaddr.sin_port));

          strcpy(send_buffer, SERV_TRSBYE_RESPONSE);
        }
        /* Query had syntax errors */
        else {
          badform = true;

          eprintf("[%s] Protocol error: Unrecognized query\n", QUERY_BADFORM);
          strcpy(send_buffer, QUERY_BADFORM);
        }

        /* Query type accepted */
        if(!badform) {
          char *token; /* For port checking, before conversion*/

          strtok(recv_buffer, "\n"); /* Remove trailing \n (terminator) */
          strtok(NULL, " "); /* Query type not needed anymore */

          language = strtok(NULL, " ");
          address = strtok(NULL, " ");
          token = strtok(NULL, " "); /* Check validity and then convert */

          /* Check if the arguments are there */
          if(language && address && token
              && (port = (unsigned short)atoi(token)) > 0) {
            int ret;

            /* Registry vs. Removal */
            if(regEntry) {
              ret = add_trs_entry(trs_list, language, address, port);
            }
            else {
              ret = remove_trs_entry(trs_list, language, address, port);
            }

            /* OK vs. NOK */
            if(ret == 0) {
              printf("[%s] Request successful! Server %s!\n",
                SERV_STATUS_OK, regEntry ? "registered" : "removed");

              sprintf(send_buffer, "%s %s", send_buffer, SERV_STATUS_OK);
            }
            else {
              printf("[%s] Request declined! Couldn\'t %s server!\n",
                SERV_STATUS_NOK, regEntry ? "register" : "remove");

              sprintf(send_buffer, "%s %s", send_buffer, SERV_STATUS_NOK);
            }
          }
          /* Protocol message unrecognized */
          else {
            eprintf("[%s] Protocol error: Unrecognized query\n", QUERY_BADFORM);
            sprintf(send_buffer, "%s %s", send_buffer, QUERY_BADFORM);
          }
        }
      }
      /* Protocol message had syntax errors */
      else {
        eprintf("[%s] Protocol error: Query had a bad form\n", QUERY_BADFORM);
        sprintf(send_buffer, "%s", QUERY_BADFORM); /* ERR */
      }

      strcat(send_buffer, "\n"); /* Terminator (cork) */

      /* Send the message along, if sendto doesn't fail */
      if(sendto(sockfd, send_buffer, strlen(send_buffer), 0,
          (struct sockaddr *)&sockaddr, addrlen) == -1) {
        perror("sendto");
        close(sockfd);
        destroy_trs_list(trs_list);
        exit(E_GENERIC);
      }
    }
  }

  /* Free resources */
  close(sockfd);
  destroy_trs_list(trs_list);

  return EXIT_SUCCESS;
}



/* Function implementations */
void printHelp(FILE *stream, const char *prog) {
  fprintf(stream, "RC Translation - TCS (Translation Contact Server)\n");
  printUsage(stream, prog);
  fprintf(stream,
    "Options:\n"
    "\t-h   Shows this help message and exits\n"
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
            TCSport = (unsigned short)val;
            ++port;
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

  return err;
}
