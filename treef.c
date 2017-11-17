#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_NODE_CAPACITY 8

struct arena {
    size_t used;
    size_t capacity;
    struct node *nodes;
};

struct node {
    const char *name;
    int sibling_idx;
    int child_idx;
};

void arena_prepare_add(struct arena *arena)
{
    if (arena->capacity == arena->used) {
        if (arena->capacity == 0)
            arena->capacity = INITIAL_NODE_CAPACITY;
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

    if (last_sibling_idx >= 0)
        arena->nodes[last_sibling_idx].sibling_idx = (int) arena->used;
    else if (parent_idx >= 0)
        arena->nodes[parent_idx].child_idx = (int) arena->used;

    return (int) arena->used++;
}

int node_add_or_find(struct arena *arena, int parent_idx, const char *name, size_t len)
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
            if (!strncmp(arena->nodes[sibling_idx].name, name, len))
                return sibling_idx;

            last_sibling_idx = sibling_idx;
            sibling_idx = arena->nodes[sibling_idx].sibling_idx;
        }
    }

    return node_add(arena, parent_idx, last_sibling_idx, strndup(name, len));
}

// Returns height at which the node was added.
size_t path_add(struct arena *arena, int parent_idx, const char *path)
{
    while (*path == '/') path++;

    // Occurs on trailing slash
    if (path[0] == '\0')
        return 0;

    char *end = strchr(path, '/');

    if (!end) {
        node_add_or_find(arena, parent_idx, path, strlen(path));
        return 1;
    } else {
        // "abc/def": end = path + 3 -> end - path = 3 -> path[end - path] = '/'
        size_t len = (size_t) (end - path);
        int new_parent_idx = node_add_or_find(arena, parent_idx, path, len);

        return 1 + path_add(arena, new_parent_idx, end + 1);
    }
}

void print_siblings(struct arena *arena, int *tree_path, size_t depth, int node_idx)
{
    int child_idx = arena->nodes[node_idx].child_idx;
    int next_idx = arena->nodes[node_idx].sibling_idx;

    for (size_t anscestor = 0; anscestor < depth; anscestor++) {
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
        tree_path[depth] = node_idx;
        print_siblings(arena, tree_path, depth + 1, child_idx);
    }

    if (next_idx >= 0) {
        print_siblings(arena, tree_path, depth, next_idx);
    }
}

void print_tree(struct arena *arena, size_t tree_height)
{
    int *path = malloc(sizeof(int) * tree_height);

    // If all paths have a common root component, we draw that as the
    // tree's root. Not if we only have a single lonely path -- that would
    // just echo that path.
    if (arena->used > 1 && arena->nodes[0].sibling_idx < 0) {
        puts(arena->nodes[0].name);
        print_siblings(arena, path, 0, 1);
    } else {
        print_siblings(arena, path, 0, 0);
    }
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
