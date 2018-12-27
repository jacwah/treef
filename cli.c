#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include "tree.h"
#include "nstr.h"
#include "color.h"

static struct nstr_block nstrb;

static void
read_input(struct tree *tree)
{
    char *line = NULL;
    size_t linelen = 0;
    ssize_t nread = 0;

    while ((nread = getline(&line, &linelen, stdin)) > 0) {
        if (line[nread - 1] == '\n')
            line[nread - 1] = '\0';

        // TODO: we know line length here; pass on
        tree_add_path(tree, &nstrb, -1, line, 0, get_type(line));
    }
}

static void
usage()
{
    fprintf(stderr, "usage: treef [-s | -S]\n");
    exit(1);
}

int
main(int argc, char *argv[])
{
    enum { OFF, ON, AUTO } flag_stat = AUTO;

    for (int i = 1; i < argc; ++i) {
        char *opt = argv[i];
        if (opt[0] == '-' && opt[1]) {
            while (*++opt) {
                switch (*opt) {
                    case 's': flag_stat = ON; break;
                    case 'S': flag_stat = OFF; break;
                    default: usage();
                }
            }
        } else {
            usage();
        }
    }
    nstr_init(&nstrb);

    if (flag_stat == AUTO && !isatty(1))
        flag_stat = OFF;
    else if (flag_stat && !color_init(&nstrb))
        flag_stat = OFF;

    struct tree tree = {0};
    read_input(&tree);

    if (tree.count)
        print_tree(&tree);

#ifdef NSTR_STATS
    nstr_print_stats(&nstrb);
#endif
}
