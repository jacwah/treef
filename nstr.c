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
    int size = (int)sizeof(struct nstr) + ALIGNUP(n, (int)sizeof(struct nstr));

    if (size > block->size - block->offset) {
#ifdef NSTR_STATS
        block->waste += block->size - block->offset;
#endif
        block->size = ALIGNUP(size, page_size);
        block->data = mmap(NULL, (size_t)block->size, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
        block->offset = 0;
    }

    result = (struct nstr *)__builtin_assume_aligned(block->data + block->offset, sizeof(struct nstr));
    block->offset += size;
#ifdef NSTR_STATS
    block->total += size;
    block->waste += n % (int)sizeof(struct nstr);
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

#ifdef NSTR_STATS
#include <stdio.h>
void
nstr_print_stats(struct nstr_block *block)
{
    int alloced = block->total + block->size - block->offset;
    int real_waste = block->waste + block->size - block->offset;
    float waste_percent = (100.0f*real_waste)/alloced;
    fprintf(stderr, "nstr stats: s=%d a=%d t=%d w=%d (%f%%)\n",
            block->size, alloced, block->total,
            real_waste, waste_percent);
}
#endif
