#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/stat.h>
#include "nstr.h"

#define ARENA_INITIAL_CAPACITY 8

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

static struct nstr *type_sgr[TYPE_COUNT];
static struct nstr empty_string;
static struct nstr *sgr0_eol;

static struct nstr_block nstrb;

/**
 * A region to store a tree of nodes.
 *
 * Using contiguous memory to store all nodes minimizes allocations and
 * provides good cache locality.
 */
struct arena {
    size_t used;
    size_t capacity;
    struct node *nodes;
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

enum { OFF, ON, AUTO } flag_stat = AUTO;

void arena_prepare_add(struct arena *arena)
{
    if (arena->capacity == arena->used) {
        if (arena->capacity == 0)
            arena->capacity = ARENA_INITIAL_CAPACITY;
        else
            arena->capacity *= 2;

        arena->nodes = realloc(
                arena->nodes,
                sizeof(struct node) * arena->capacity);
    }
}

static int
node_add(struct arena *arena, int parent_idx, int last_sibling_idx, struct nstr *name, enum type type)
{
    arena_prepare_add(arena);

    struct node *new = &arena->nodes[arena->used];
    new->name = name;
    new->sibling_idx = -1;
    new->child_idx = -1;
    new->type = type;

    // Link node into the tree
    if (last_sibling_idx >= 0)
        arena->nodes[last_sibling_idx].sibling_idx = (int) arena->used;
    else if (parent_idx >= 0)
        arena->nodes[parent_idx].child_idx = (int) arena->used;

    return (int) arena->used++;
}

struct find_result {
    int node_idx;
    int last_sibling_idx;
};

static struct find_result
node_find(struct arena *arena, int parent_idx, char *name, int len)
{
    struct find_result result;
    result.last_sibling_idx = -1;

    if (arena->used) {
        int sibling_idx = -1;

        if (parent_idx >= 0)
            sibling_idx = arena->nodes[parent_idx].child_idx;
        else
            sibling_idx = 0;

        while (sibling_idx >= 0) {
            // If this node already exists, no need to add it again
            struct nstr *sibling_name = arena->nodes[sibling_idx].name;
            if (sibling_name->n == len && !memcmp(sibling_name->str, name, (size_t)len)) {
                result.node_idx = sibling_idx;
                return result;
            }

            result.last_sibling_idx = sibling_idx;
            sibling_idx = arena->nodes[sibling_idx].sibling_idx;
        }
    }

    result.node_idx = -1;
    return result;
}

int
node_find_or_add(struct arena *arena, int parent_idx, char *name, int len, enum type type)
{
    struct find_result found = node_find(arena, parent_idx, name, len);
    if (found.node_idx >= 0)
        return found.node_idx;
    else
        return node_add(arena, parent_idx, found.last_sibling_idx, nstr_dup(&nstrb, len, name), type);
}

enum type
get_type(const char *path)
{
    if (flag_stat) {
        struct stat st;
        if (lstat(path, &st)) {
            return TYPE_NONE;
        } else {
            switch (S_IFMT & st.st_mode) {
                case S_IFIFO:
                    return TYPE_PIPE;
                case S_IFCHR:
                    return TYPE_CHAR;
                case S_IFDIR:
                    // TODO: sticky, ow
                    return TYPE_DIR;
                case S_IFBLK:
                    return TYPE_BLOCK;
                case S_IFREG:
                    // GNU checks in this order - seems arbitrary?
                    if (S_IXUSR & st.st_mode)
                        if (S_ISUID & st.st_mode)
                            return TYPE_SUID;
                        else if (S_ISGID & st.st_mode)
                            return TYPE_SGID;
                        else
                            return TYPE_EXEC;
                    else
                        return TYPE_FILE;
                case S_IFLNK:
                    // TODO: orphan?
                    return TYPE_LINK;
                case S_IFSOCK:
                    return TYPE_SOCK;
                //case S_IFWHT: whiteout
                default:
                    return TYPE_NONE;
            }
        }
    } else {
        return TYPE_NONE;
    }
}

/**
 * Add a file path to the tree.
 *
 * @return Height of the last component in path.
 */
size_t path_add(struct arena *arena, int parent_idx, char *path, int off, enum type type)
{
    while (path[off] == '/') off++;

    // Occurs on trailing slash
    if (path[off] == '\0')
        return 0;

    char *slash = strchr(path+off, '/');

    if (!slash) {
        // Leaf node
        // TODO strchr with len
        node_find_or_add(arena, parent_idx, path+off, (int)strlen(path+off), type);
        return 1;
    } else {
        int grandparent_idx = parent_idx;
        int len = (int)(slash-path)-off;
        struct find_result found_parent = node_find(arena, grandparent_idx, path+off, len);

        if (found_parent.node_idx >= 0) {
            parent_idx = found_parent.node_idx;
        } else {
            enum type parent_type;
            *slash = '\0';
            parent_type = get_type(path);
            *slash = '/';
            parent_idx = node_add(arena, grandparent_idx, found_parent.last_sibling_idx, nstr_dup(&nstrb, len, path+off), parent_type);
        }

        return 1 + path_add(arena, parent_idx, path, off+len+1, type);
    }
}

static bool
parse_gnu_colors(char *colors)
{
    puts(colors);
    return false;
}

static struct nstr *
bsd_sgr(char f, char b)
{
    if (f == 'x' && b == 'x')
        return &empty_string;
    // ESC [ff;bb;am
    struct nstr *sgr = nstr_alloc(&nstrb, 10);

    sgr->str[sgr->n++] = '\x1b';
    sgr->str[sgr->n++] = '[';

    if (isupper(f)) {
        sgr->str[sgr->n++] = '1';
    }

    if (f != 'x') {
        char color;

        if (sgr->n != 2)
            sgr->str[sgr->n++] = ';';

        switch (f) {
            case 'a': color = '0'; break;
            case 'b': color = '1'; break;
            case 'c': color = '2'; break;
            case 'd': color = '3'; break;
            case 'e': color = '4'; break;
            case 'f': color = '5'; break;
            case 'g': color = '6'; break;
            case 'h': color = '7'; break;
        }
        sgr->str[sgr->n++] = '3';
        sgr->str[sgr->n++] = color;
    }

    if (b != 'x') {
        char color;

        if (sgr->n != 2)
            sgr->str[sgr->n++] = ';';

        switch (b) {
            case 'a': color = '0'; break;
            case 'b': color = '1'; break;
            case 'c': color = '2'; break;
            case 'd': color = '3'; break;
            case 'e': color = '4'; break;
            case 'f': color = '5'; break;
            case 'g': color = '6'; break;
            case 'h': color = '7'; break;
        }
        sgr->str[sgr->n++] = '4';
        sgr->str[sgr->n++] = color;
    }

    sgr->str[sgr->n++] = 'm';
    return sgr;
}

static bool
parse_bsd_colors(const char *colors)
{
    /* From man ls: */

    /*
    1.	directory
    2.	symbolic link
    3.	socket
    4.	pipe
    5.	executable
    6.	block special
    7.	character special
    8.	executable with setuid bit set
    9.	executable with setgid bit set
    10.	directory writable to others, with sticky bit
    11.	directory writable to others, without sticky bit
    */

    /*
    a	 black
    b	 red
    c	 green
    d	 brown
    e	 blue
    f	 magenta
    g	 cyan
    h	 light grey
    A	 bold black, usually shows up as dark grey
    B	 bold red
    C	 bold green
    D	 bold brown, usually shows up as yellow
    E	 bold blue
    F	 bold magenta
    G	 bold cyan
    H	 bold light grey; looks like bright white
    x	 default foreground or background
    */

    int len = 0;
    while (len < 2*11) {
        if (!colors[len] || !colors[len+1])
            break;
        if (colors[len] != 'x' && (tolower(colors[len]) < 'a' || tolower(colors[len]) > 'h'))
            break;
        if (colors[len+1] != 'x' && (colors[len+1] < 'a' || colors[len+1] > 'h'))
            break;
        len += 2;
    }

    if (len == 2*11) {
        type_sgr[TYPE_DIR]  = bsd_sgr(colors[0],  colors[1]);
        type_sgr[TYPE_LINK] = bsd_sgr(colors[2],  colors[3]);
        type_sgr[TYPE_SOCK] = bsd_sgr(colors[4],  colors[5]);
        type_sgr[TYPE_PIPE] = bsd_sgr(colors[6],  colors[7]);
        type_sgr[TYPE_EXEC] = bsd_sgr(colors[8],  colors[9]);
        type_sgr[TYPE_BLOCK]= bsd_sgr(colors[10], colors[11]);
        type_sgr[TYPE_CHAR] = bsd_sgr(colors[12], colors[13]);
        type_sgr[TYPE_SUID] = bsd_sgr(colors[14], colors[15]);
        type_sgr[TYPE_SGID] = bsd_sgr(colors[16], colors[17]);
        type_sgr[TYPE_OTHRS]= bsd_sgr(colors[18], colors[19]);
        type_sgr[TYPE_OTHR] = bsd_sgr(colors[20], colors[21]);
        return true;
    } else {
        fprintf(stderr, "warning: invalid LSCOLORS value\n");
        return false;
    }
}

static bool
set_default_bsd_colors()
{
    return parse_bsd_colors("exfxcxdxbxegedabagacad");
}

static bool
sgr_init()
{
    for (int i = 0; i < TYPE_COUNT; ++i)
        type_sgr[i] = &empty_string;

    sgr0_eol = nstr_alloc(&nstrb, 4);
    sgr0_eol->str[0] = '\x1b';
    sgr0_eol->str[1] = '[';
    sgr0_eol->str[2] = 'm';
    sgr0_eol->str[3] = '\n';
    sgr0_eol->n = 4;

    char *env;
    if ((env = getenv("LS_COLORS")) && *env && parse_gnu_colors(env))
        return true;
    else if ((env = getenv("LSCOLORS")) && *env && parse_bsd_colors(env))
        return true;
    else if (getenv("CLICOLOR"))
        return set_default_bsd_colors();
#if 0
    else if (getenv("TERMCOLOR"))
        return set_default_gnu_colors;
#endif
    else
        return false;
}

void
print_node(struct node node)
{
    if (flag_stat) {
        fwrite(type_sgr[node.type]->str, 1, type_sgr[node.type]->n, stdout);
        fwrite(node.name->str, 1, node.name->n, stdout);
        fwrite(sgr0_eol->str, 1, sgr0_eol->n, stdout);

    } else {
        fwrite(node.name->str, 1, node.name->n, stdout);
        putc('\n', stdout);
    }
}

/**
 * Print the nodes in a tree style.
 *
 * tree_path is used to track the path of parents from the current node to the
 * root node and must have capactiy for the longest path from leaf to root (the
 * height of the tree).
 */
void print_nodes(struct arena *arena, int *tree_path, int depth, int node_idx)
{
    for (;;) {
        struct node node = arena->nodes[node_idx];
        int child_idx = node.child_idx;
        int next_idx = node.sibling_idx;

        for (int anscestor = 0; anscestor < depth; anscestor++) {
            if (arena->nodes[tree_path[anscestor]].sibling_idx < 0)
                printf("    ");
            else
                printf("│   ");
        }

        if (next_idx < 0) {
            printf("└── ");
        } else {
            printf("├── ");
        }

        print_node(node);

        if (child_idx >= 0) {
            tree_path[depth++] = node_idx;
            node_idx = child_idx;
        } else {
            while (next_idx < 0 && depth > 0) {
                node_idx = tree_path[--depth];
                next_idx = arena->nodes[node_idx].sibling_idx;
            }
            if (next_idx < 0)
                break;
            node_idx = next_idx;
        }
    }
}

void print_tree(struct arena *arena, size_t tree_height)
{
    int *path = malloc(sizeof(int) * tree_height);
    int root_idx;

    // If all paths have a common root component, we draw that as the
    // tree's root. Not if we only have a single lonely path -- that would
    // just echo that path.
    if (arena->used > 1 && arena->nodes[0].sibling_idx < 0) {
        print_node(arena->nodes[0]);
        root_idx = 1;
    } else {
        root_idx = 0;
    }

    print_nodes(arena, path, 0, root_idx);
}

size_t
read_input(struct arena *arena)
{
    char *line = NULL;
    size_t linelen = 0;
    ssize_t nread = 0;
    size_t tree_height = 1;

    while ((nread = getline(&line, &linelen, stdin)) > 0) {
        if (line[nread - 1] == '\n')
            line[nread - 1] = '\0';

        // TODO: we know line length here; pass on
        size_t height = path_add(arena, -1, line, 0, get_type(line));

        if (height > tree_height)
            tree_height = height;
    }

    return tree_height;
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
    else if (flag_stat && !sgr_init())
        flag_stat = OFF;

    struct arena arena;
    memset(&arena, 0, sizeof(struct arena));
    size_t tree_height = read_input(&arena);

    if (arena.used)
        print_tree(&arena, tree_height);

#ifdef NSTR_STATS
    nstr_print_stats(&nstrb);
#endif
}
