#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>

#define ARENA_INITIAL_CAPACITY 8

#if S_IFMT == 0170000
#define MODE_SHIFT 12
#else
#error "Hardcoded S_IFMT does not match"
#endif

#define MODE(st_mode)((S_IFMT & (st_mode)) >> MODE_SHIFT)
#define CSI "\x1b["

// Do this differently on Mac/BSD and GNU/Linux?
static const char *mode_sgr[S_IFMT >> MODE_SHIFT];
const char *sgr0 = CSI "m";
const char *empty_string = "";

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
    const char *name;
    int sibling_idx;
    int child_idx;
    int mode;
};

int flag_stat = 0;

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

int node_add(struct arena *arena, int parent_idx, int last_sibling_idx, const char *name, int mode)
{
    arena_prepare_add(arena);

    struct node *new = &arena->nodes[arena->used];
    new->name = name;
    new->sibling_idx = -1;
    new->child_idx = -1;
    new->mode = mode;

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

struct find_result
node_find(struct arena *arena, int parent_idx, const char *name, size_t len)
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
            const char *sibling_name = arena->nodes[sibling_idx].name;
            if (!strncmp(sibling_name, name, len) && sibling_name[len] == '\0') {
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
node_find_or_add(struct arena *arena, int parent_idx, const char *name, size_t len, int mode)
{
    struct find_result found = node_find(arena, parent_idx, name, len);
    if (found.node_idx >= 0)
        return found.node_idx;
    else
        return node_add(arena, parent_idx, found.last_sibling_idx, strndup(name, len), mode);
}

int
stat_mode(const char *path)
{
    if (flag_stat) {
        struct stat st;
        if (lstat(path, &st))
            return 0;
        else
            return MODE(st.st_mode);
    } else {
        return 0;
    }
}

/**
 * Add a file path to the tree.
 *
 * @return Height of the last component in path.
 */
size_t path_add(struct arena *arena, int parent_idx, char *path, size_t off, int mode)
{
    while (path[off] == '/') off++;

    // Occurs on trailing slash
    if (path[off] == '\0')
        return 0;

    char *slash = strchr(path+off, '/');

    if (!slash) {
        // Leaf node
        node_find_or_add(arena, parent_idx, path+off, strlen(path+off), mode);
        return 1;
    } else {
        // "abc/def": end = path + 3 -> end - path = 3 -> path[end - path] = '/'
        int grandparent_idx = parent_idx;
        size_t len = (size_t)(slash - (path + off));
        struct find_result found_parent = node_find(arena, grandparent_idx, path+off, len);

        if (found_parent.node_idx >= 0) {
            parent_idx = found_parent.node_idx;
        } else {
            int parent_mode;
            *slash = '\0';
            parent_mode = stat_mode(path);
            *slash = '/';
            parent_idx = node_add(arena, grandparent_idx, found_parent.last_sibling_idx, strndup(path+off, len), parent_mode);
        }

        return 1 + path_add(arena, parent_idx, path, off+len+1, mode);
    }
}

static void
parse_gnu_colors(char *colors)
{
    puts(colors);
}

static char *
bsd_sgr(char f, char b)
{
    int bold = isupper(f);
    int setf = f != 'x';
    int setb = b != 'x';
    int size = 2+(bold+setf+setb)*3+1;
    char *sgr = malloc((size_t)size);
    char *aptr = sgr;
    strcpy(aptr, CSI);
    aptr += 2;
    char *ptr = aptr;
    const char *attr = NULL;
    f = (char)tolower(f);

    if (setf) {
        switch (f) {
            case 'a': attr = "30"; break;
            case 'b': attr = "31"; break;
            case 'c': attr = "32"; break;
            case 'd': attr = "33"; break;
            case 'e': attr = "34"; break;
            case 'f': attr = "35"; break;
            case 'g': attr = "36"; break;
            case 'h': attr = "37"; break;
        }
        strcpy(ptr, attr);
        ptr += 2;
    }

    if (setb) {
        switch (f) {
            case 'a': attr = "40"; break;
            case 'b': attr = "41"; break;
            case 'c': attr = "42"; break;
            case 'd': attr = "43"; break;
            case 'e': attr = "44"; break;
            case 'f': attr = "45"; break;
            case 'g': attr = "46"; break;
            case 'h': attr = "47"; break;
        }
        if (ptr != aptr) {
            *ptr = ';';
            ++ptr;
        }
        strcpy(ptr, attr);
        ptr += 2;
    }

    if (bold) {
        if (ptr != aptr) {
            *ptr = ';';
            ++ptr;
        }
        *ptr = '1';
        ++ptr;
    }

    *ptr = 'm';
    ++ptr;
    *ptr = '\0';
    return sgr;
}

static void
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
        mode_sgr[MODE(S_IFDIR)] = bsd_sgr(colors[0],  colors[1]);
        mode_sgr[MODE(S_IFLNK)] = bsd_sgr(colors[2],  colors[3]);
        mode_sgr[MODE(S_IFSOCK)]= bsd_sgr(colors[4],  colors[5]);
        mode_sgr[MODE(S_IFIFO)] = bsd_sgr(colors[6],  colors[7]);
        /*
        mode_sgr[MODE(S_IFDIR)] = bsd_sgr(colors[8],  colors[9]);
        mode_sgr[MODE(S_IFDIR)] = bsd_sgr(colors[10], colors[11]);
        mode_sgr[MODE(S_IFDIR)] = bsd_sgr(colors[12], colors[13]);
        mode_sgr[MODE(S_IFDIR)] = bsd_sgr(colors[14], colors[15]);
        mode_sgr[MODE(S_IFDIR)] = bsd_sgr(colors[16], colors[17]);
        mode_sgr[MODE(S_IFDIR)] = bsd_sgr(colors[18], colors[19]);
        mode_sgr[MODE(S_IFDIR)] = bsd_sgr(colors[20], colors[21]);
        */
    } else {
        fprintf(stderr, "warning: invalid LSCOLORS value\n");
    }
}

static void
set_default_bsd_colors()
{
    parse_bsd_colors("exfxcxdxbxegedabagacad");
}

static void
sgr_init()
{
    char *gnucolors = getenv("LS_COLORS");
    char *bsdcolors = getenv("LSCOLORS");;
    char *clicolor = getenv("CLICOLOR");;

    for (int i = 0; i < (S_IFMT >> MODE_SHIFT); ++i)
        mode_sgr[i] = empty_string;

    if (gnucolors && *gnucolors)
        parse_gnu_colors(gnucolors);
    else if (bsdcolors && *bsdcolors)
        parse_bsd_colors(bsdcolors);
    else if (clicolor)
        set_default_bsd_colors();
}

void
print_node(struct node node)
{
    if (flag_stat)
        printf("%s%s%s\n", mode_sgr[node.mode], node.name, sgr0);
    else
        printf("%s\n", node.name);
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

        size_t height = path_add(arena, -1, line, 0, stat_mode(line));

        if (height > tree_height)
            tree_height = height;
    }

    return tree_height;
}

int
main(int argc, char *argv[])
{
    if (argc == 2 && !strcmp(argv[1], "-s")) {
        flag_stat = 1;
        sgr_init();
    } else if (argc > 1) {
        fprintf(stderr, "usage: treef [-s]\n");
        return 1;
    }

    struct arena arena;
    memset(&arena, 0, sizeof(struct arena));
    size_t tree_height = read_input(&arena);

    if (arena.used)
        print_tree(&arena, tree_height);
}
