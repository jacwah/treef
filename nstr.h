#include <stdint.h>

struct nstr {
    uint16_t n;
    char str[];
};

struct nstr_block {
    char *data;
    int size;
    int offset;
#ifdef NSTR_STATS
    int waste;
    int total;
#endif
};

void
nstr_init(struct nstr_block *block);

struct nstr *
nstr_alloc(struct nstr_block *block, int n);

struct nstr *
nstr_dup(struct nstr_block *block, int n, char *str);

#ifdef NSTR_STATS
void
nstr_print_stats(struct nstr_block *block);
#endif
