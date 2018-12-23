#include "nstr.h"
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>

#define ALIGNUP(addr, align) (((addr)+((align)-1))&-(align))

static int page_size;

void
nstr_init(struct nstr_block *block)
{
    page_size = getpagesize();
    block->size = page_size;
    block->data = mmap(NULL, (size_t)block->size, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
    block->offset = 0;
#ifdef NSTR_STATS
    block->waste = 0;
    block->total = 0;
#endif
}

struct nstr *
nstr_alloc(struct nstr_block *block, int n)
{
    struct nstr *result;
    // Align to size of nstr.n
    int size = 2 + ALIGNUP(n, 2);

    if (size > block->size - block->offset) {
#ifdef NSTR_STATS
        block->waste += block->size - block->offset;
#endif
        block->size = ALIGNUP(size, page_size);
        block->data = mmap(NULL, (size_t)block->size, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
        block->offset = 0;
    }

    result = (struct nstr *)__builtin_assume_aligned(block->data + block->offset, 2);
    result->n = 0;
    block->offset += size;
#ifdef NSTR_STATS
    block->total += size;
#endif
    return result;
}

struct nstr *
nstr_dup(struct nstr_block *block, int n, char *str)
{
    struct nstr *ns = nstr_alloc(block, n);
    memcpy(&ns->str, str, n);
    ns->n = (uint16_t)n;
    return ns;
}
