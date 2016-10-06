typedef struct trs_entry trs_entry_t;
typedef struct trs_list trs_list_t;

trs_list_t *new_trs_list();

void add_trs_entry(const char *language, const char *address,
  unsigned short port);

void remove_trs_entry(const char *language);

trs_entry_t *get_trs_entry(const char *language);

void destroy(trs_list_t *trs_list);
