/**
 * RC Translation - RC@IST/UL
 * TRS list implementation - RC45179 16'17
 *
 * @author: Rui Ventura (ist181045)
 * @author: Diogo Freitas (ist181586)
 * @author: Sara Azinhal (ist181700)
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "trs_list.h"


trs_list_t *new_trs_list() {
  trs_list_t *trs_list = (trs_list_t *)malloc(sizeof(struct trs_list));
  if(trs_list) {
    trs_list->size = 0;
    trs_list->head = NULL;
  }

  return trs_list;
}

int add_trs_entry(trs_list_t *trs_list, const char *language,
    const char *address, unsigned short port) {
  bool canAdd = true;
  bool lang, addr, fport;
  trs_entry_t *node = trs_list->head;

  while(node && canAdd) {
    lang = !strncmp(node->language, language, LANG_MAX_LEN);
    addr = !strcmp(node->address, address);
    fport = node->port == port;

    canAdd = !lang && (!addr || !fport);
    node = node->next;
  }
  /* trs_entry wasn't found, add it */
  if(canAdd) {
    node = (trs_entry_t *)malloc(sizeof(struct trs_entry));
    if(node) {
      strncpy(node->language, language, LANG_MAX_LEN);
      node->address = strdup(address);
      node->port = port;
      node->next = trs_list->head;
      trs_list->head = node;

      trs_list->size++;
      return 0;
    }
  }

  return -1;
}

int remove_trs_entry(trs_list_t *trs_list, const char *language,
  const char *address, unsigned short port) {
  trs_entry_t *node = trs_list->head, *temp = NULL;

  while(node
      && (strncmp(node->language, language, LANG_MAX_LEN)
      || strcmp(node->address, address)
      || node->port != port)) {
    temp = node;
    node = node->next;
  }
  /* remove trs_entry if it exists */
  if(node) {
    if(node == trs_list->head)
      trs_list->head = node->next;
    else
      temp->next = node->next;

    free(node->address);
    free(node);
    node = NULL;

    trs_list->size--;
    return 0;
  }

  return -1;
}

trs_entry_t *get_trs_entry_lang(trs_list_t *trs_list, const char *language) {
  trs_entry_t *node = trs_list->head;
  while(node && strncmp(node->language, language, LANG_MAX_LEN)) {
    node = node->next;
  }
  return node;
}

trs_entry_t *get_trs_entry_addr(trs_list_t *trs_list, const char *address) {
  trs_entry_t *node = trs_list->head;
  while(node && strcmp(node->address, address)) {
    node = node->next;
  }
  return node;
}

size_t destroy_trs_list(trs_list_t *trs_list) {
  size_t size = trs_list->size;
  trs_entry_t *node = trs_list->head;

  while(node) {
    trs_entry_t *temp = node;

    node = node->next;
    free(temp->address);
    free(temp);

    temp = NULL;
  }

  free(trs_list);
  trs_list = NULL;

  return size;
}
