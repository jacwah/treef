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
    size_t sibling_idx;
    size_t child_idx;
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

size_t node_add(struct arena *arena, size_t parent_idx, const char *name, size_t last_sibling_idx)
{
    arena_prepare_add(arena);

    struct node *new = &arena->nodes[arena->used];
    new->name = name;
    new->sibling_idx = 0;
    new->child_idx = 0;

    if (last_sibling_idx)
        arena->nodes[last_sibling_idx].sibling_idx = arena->used;
    else if (arena->used)
        arena->nodes[parent_idx].child_idx = arena->used;

    return arena->used++;;
}

size_t node_add_or_find(struct arena *arena, size_t parent_idx, const char *name, size_t len)
{
    size_t last_sibling_idx = 0;

    if (arena->used) {
        size_t sibling_idx = arena->nodes[parent_idx].child_idx;

        while (sibling_idx) {
            // If this node already exists, no need to add it again
            if (!strncmp(arena->nodes[sibling_idx].name, name, len))
                return sibling_idx;

            last_sibling_idx = sibling_idx;
            sibling_idx = arena->nodes[sibling_idx].sibling_idx;
        }
    }

    return node_add(arena, parent_idx, strndup(name, len), last_sibling_idx);
}

// Returns height at which the node was added.
size_t path_add(struct arena *arena, size_t parent_idx, const char *path)
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
        size_t new_parent_idx = node_add_or_find(arena, parent_idx, path, len);

        return 1 + path_add(arena, new_parent_idx, end + 1);
    }
}

void print_tree(struct arena *arena, size_t *root_path, size_t depth, size_t root_idx)
{
    size_t child_idx = arena->nodes[root_idx].child_idx;

    if (child_idx)
        root_path[depth] = root_idx;

    while (child_idx) {
        size_t next_idx = arena->nodes[child_idx].sibling_idx;

        for (size_t anscestor = 1; anscestor <= depth; anscestor++) {
            if (arena->nodes[root_path[anscestor]].sibling_idx)
                printf("│   ");
            else
                printf("    ");
        }

        if (next_idx) {
            printf("├── ");
        } else {
            printf("└── ");
        }

        printf("%s\n", arena->nodes[child_idx].name);
        print_tree(arena, root_path, depth + 1, child_idx);
        child_idx = next_idx;
    }
}

int main()
{
    char *line = NULL;
    size_t linelen = 0;
    ssize_t nread = 0;
    size_t tree_height = 1;
    struct arena arena;

    memset(&arena, 0, sizeof(arena));
    node_add_or_find(&arena, 0, NULL, 0);

    while ((nread = getline(&line, &linelen, stdin)) > 0) {
        if (line[nread - 1] == '\n')
            line[nread - 1] = '\0';

        size_t height = path_add(&arena, 0, line);

        if (height > tree_height)
            tree_height = height;
    }

    size_t *path = malloc(sizeof(size_t) * tree_height);

    // If all paths have a common root component, we draw that as the
    // tree's root. Not if we only have a single lonely path -- that would
    // just echo that path.
    if (arena.used > 2 && !arena.nodes[1].sibling_idx) {
        puts(arena.nodes[1].name);
        print_tree(&arena, path, 0, 1);
    } else {
        print_tree(&arena, path, 0, 0);
    }
}
