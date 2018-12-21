#include <stdint.h>
struct nstr {
    uint16_t n;
    char str[1];
};

struct nstr_block {
    char *data;
    int size;
    int offset;
    int waste;
};

void
nstr_init(struct nstr_block *block);

struct nstr *
nstr_alloc(struct nstr_block *block, int n);
