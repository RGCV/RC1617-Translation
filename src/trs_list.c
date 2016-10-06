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
