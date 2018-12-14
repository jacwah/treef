#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARENA_INITIAL_CAPACITY 8

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
};

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

int node_add(struct arena *arena, int parent_idx, int last_sibling_idx, const char *name)
{
    arena_prepare_add(arena);

    struct node *new = &arena->nodes[arena->used];
    new->name = name;
    new->sibling_idx = -1;
    new->child_idx = -1;

    // Link node into the tree
    if (last_sibling_idx >= 0)
        arena->nodes[last_sibling_idx].sibling_idx = (int) arena->used;
    else if (parent_idx >= 0)
        arena->nodes[parent_idx].child_idx = (int) arena->used;

    return (int) arena->used++;
}

int node_find_or_add(struct arena *arena, int parent_idx, const char *name, size_t len)
{
    int last_sibling_idx = -1;

    if (arena->used) {
        int sibling_idx = -1;

        if (parent_idx >= 0)
            sibling_idx = arena->nodes[parent_idx].child_idx;
        else
            sibling_idx = 0;

        while (sibling_idx >= 0) {
            // If this node already exists, no need to add it again
            const char *sibling_name = arena->nodes[sibling_idx].name;
            if (!strncmp(sibling_name, name, len)
                && sibling_name[len] == '\0')
                return sibling_idx;

            last_sibling_idx = sibling_idx;
            sibling_idx = arena->nodes[sibling_idx].sibling_idx;
        }
    }

    return node_add(arena, parent_idx, last_sibling_idx, strndup(name, len));
}

/**
 * Add a file path to the tree.
 *
 * @return Height of the last component in path.
 */
size_t path_add(struct arena *arena, int parent_idx, const char *path)
{
    while (*path == '/') path++;

    // Occurs on trailing slash
    if (path[0] == '\0')
        return 0;

    char *end = strchr(path, '/');

    if (!end) {
        node_find_or_add(arena, parent_idx, path, strlen(path));
        return 1;
    } else {
        // "abc/def": end = path + 3 -> end - path = 3 -> path[end - path] = '/'
        size_t len = (size_t) (end - path);
        int new_parent_idx = node_find_or_add(arena, parent_idx, path, len);

        return 1 + path_add(arena, new_parent_idx, end + 1);
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
        int child_idx = arena->nodes[node_idx].child_idx;
        int next_idx = arena->nodes[node_idx].sibling_idx;

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

        printf("%s\n", arena->nodes[node_idx].name);

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
        puts(arena->nodes[0].name);
        root_idx = 1;
    } else {
        root_idx = 0;
    }

    print_nodes(arena, path, 0, root_idx);
}

int main()
{
    char *line = NULL;
    size_t linelen = 0;
    ssize_t nread = 0;
    size_t tree_height = 1;
    struct arena arena;

    memset(&arena, 0, sizeof(struct arena));

    while ((nread = getline(&line, &linelen, stdin)) > 0) {
        if (line[nread - 1] == '\n')
            line[nread - 1] = '\0';

        size_t height = path_add(&arena, -1, line);

        if (height > tree_height)
            tree_height = height;
    }

    if (arena.used)
        print_tree(&arena, tree_height);
}
