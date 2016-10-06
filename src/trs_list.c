/**
 * RC Translation - RC@IST/UL
 * User client - RC45179 15'16
 *
 * @author: Rui Ventura (ist181045)
 * @author: Diogo Freitas (ist181586)
 * @author: Sara Azinhal (ist181700)
 */

#include "rctr.h"
#include "trs_list.h"

struct trs_entry {
  char language[LANG_MAX_LEN];
  char *address;
  unsigned int port;
  struct trs_entry *next;
};

struct trs_list {
  struct trs_entry *head;
};

trs_list_t *new_trs_list() {
  trs_list_t *trs_list = (trs_list_t *)malloc(sizeof(struct trs_list));
  if(trs_list) {
    trs_list->head = NULL;
  }

  return trs_list;
}

void add_trs_entry(trs_list_t *trs_list, const char *language,
    const char *address, unsigned short port) {
  trs_entry_t *node = trs_list->head, *temp = NULL;

  while(node) {
    temp = node;
    node = node->next;
  }
  node = (trs_entry_t *)malloc(sizeof(struct trs_entry));
  if(node) {
    strncpy(node->language, language, LANG_MAX_LEN);
    node->address = strdup(address);
    node->port = port;
    node->next = NULL;

    if(temp) temp->next = node;
  }
}

void remove_trs_entry(trs_list_t *trs_list, const char *language) {
  trs_entry_t *node = trs_list->head, *temp = NULL;

  while(node && strncmp(node->language, language, LANG_MAX_LEN)) {
    temp = node;
    node = node->next;
  }
  if(node) {
    if(node != temp)
      temp->next = node->next;
    free(node->address);
    free(node);
  }
}

trs_entry_t *get_trs_entry(trs_list_t *trs_list, const char *language) {
  trs_entry_t *node = trs_list->head;
  while(node && strncmp(node->language, language, LANG_MAX_LEN));
  return node;
}

void destroy_trs_list(trs_list_t *trs_list) {
  trs_entry_t *node = trs_list->head;

  while(node) {
    trs_entry_t *temp = node;

    node = node->next;
    free(temp->language);
    free(temp);
  }

  free(trs_list);
}
