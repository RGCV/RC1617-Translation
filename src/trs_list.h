/**
 * RC Translation - RC@IST/UL
 * User client - RC45179 15'16
 *
 * @author: Rui Ventura (ist181045)
 * @author: Diogo Freitas (ist181586)
 * @author: Sara Azinhal (ist181700)
 */

typedef struct trs_entry trs_entry_t;
typedef struct trs_list trs_list_t;

trs_list_t *new_trs_list();

void add_trs_entry(trs_list_t *trs_list, const char *language,
  const char *address, unsigned short port);

void remove_trs_entry(trs_list_t *trs_list, const char *language);

trs_entry_t *get_trs_entry(trs_list_t *trs_list, const char *language);

void destroy_trs_list(trs_list_t *trs_list);
