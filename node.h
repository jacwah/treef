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
    //TYPE_ORPHAN,
    TYPE_EXEC,
    TYPE_SUID,
    TYPE_SGID,
    //TYPE_STICKY,
    TYPE_OTHR,
    TYPE_OTHRS,
    TYPE_COUNT
};

/**
 * A node in the tree.
 *
 * Refers to its first child and next siblings by indices into the containing
 * arena, or -1 if none exists. Nodes form a left-child right-sibling binary
 * tree.
 */
struct node {
    struct nstr *name;
    int sibling_idx;
    int child_idx;
    enum type type;
};
