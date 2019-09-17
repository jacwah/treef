#pragma once

#include "tree.h"
#include "nstr.h"

#include <stdbool.h>

enum type
get_type(const char *);

bool
color_init(struct nstr_block *);

void
print_node(struct node node);
