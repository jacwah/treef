#pragma once

#include "nstr.h"

enum type {
    TYPE_NONE,
    TYPE_FILE,
    TYPE_DIR,
    TYPE_LINK,
    TYPE_PIPE,
    TYPE_SOCK,
    TYPE_BLOCK,
    TYPE_CHAR,
    TYPE_ORPHAN,
    TYPE_EXEC,
    TYPE_SUID,
    TYPE_SGID,
    TYPE_STICKY,
    TYPE_OWRITS,
    TYPE_OWRITE,
    TYPE_COUNT
};

struct node {
    struct nstr *name;
    int sibling_idx;
    int child_idx;
    enum type type;
};

struct tree {
    size_t count;
    size_t capacity;
    size_t height;
    struct node *nodes;
};

void
tree_add_path(struct tree *, struct nstr_block *, int, char *, size_t, enum type);

void
print_tree(struct tree *);
