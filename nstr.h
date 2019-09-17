#pragma once

#include <stdint.h>
#include <sys/types.h>

typedef uint16_t nstrlen;

struct nstr {
    nstrlen n;
    char str[];
};

struct nstr_block {
    char *data;
    size_t size;
    size_t offset;
#ifdef NSTR_STATS
    size_t waste;
    size_t total;
#endif
};

void
nstr_init(struct nstr_block *block);

struct nstr *
nstr_alloc(struct nstr_block *block, nstrlen n);

struct nstr *
nstr_dup(struct nstr_block *block, nstrlen n, char *str);

#ifdef NSTR_STATS
void
nstr_print_stats(struct nstr_block *block);
#endif
