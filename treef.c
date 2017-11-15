#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct node {
    const char *name;
    size_t childcount;
    size_t capacity;
    struct node **children;
};

struct node *node_add(struct node *parent, char *name)
{
    // De-dupe nodes
    for (size_t i = 0; i < parent->childcount; i++) {
        if (!strcmp(parent->children[i]->name, name)) {
            return parent->children[i];
        }
    }

    struct node *node = malloc(sizeof(struct node));

    node->name = strdup(name);
    node->childcount = 0;
    node->capacity = 0;
    node->children = NULL;

    if (parent->capacity == parent->childcount) {
        if (parent->capacity == 0)
            parent->capacity = 1;
        else
            parent->capacity *= 2;

        parent->children = realloc(
                parent->children,
                sizeof(struct node *) * parent->capacity);
    }

    parent->children[parent->childcount++] = node;

    return node;
}

// Returns height at which the node was added.
size_t path_add(struct node *root, char *path)
{
    while (*path == '/') path++;

    // Occurs on trailing slash
    if (path[0] == '\0')
        return 0;

    char *end = strchr(path, '/');

    if (!end) {
        node_add(root, path);
        return 1;
    } else {
        // "abc/def": end = path + 3 -> end - path = 3 -> path[end - path] = '/'
        size_t len = (size_t) (end - path);

        for (size_t i = 0; i < root->childcount; i++) {
            if (!strncmp(root->children[i]->name, path, len)) {
                return 1 + path_add(root->children[i], end + 1);
            }
        }

        char *name = malloc(len + 1);
        memcpy(name, path, len);
        name[len] = '\0';

        struct node *parent = node_add(root, name);
        return 1 + path_add(parent, end + 1);
    }
}

void print_tree(struct node *root, size_t *toddlers, size_t depth)
{
    // toddlers are unprocessed children
    toddlers[depth] = root->childcount;

    for (size_t i = 0; i < root->childcount; i++) {
        // Indent
        for (size_t level = 0; level < depth; level++) {
            if (toddlers[level] > 0)
                printf("│   ");
            else
                printf("    ");
        }

        if (toddlers[depth] > 1)
            printf("├── ");
        else
            printf("└── ");

        printf("%s\n", root->children[i]->name);

        toddlers[depth]--;

        if (root->children[i]->childcount)
            print_tree(root->children[i], toddlers, depth + 1);
    }
}

int main()
{
    char *line = NULL;
    size_t linelen = 0;
    ssize_t nread = 0;
    size_t tree_height = 1;
    struct node *root = malloc(sizeof(struct node));

    root->name = "";
    root->childcount = 0;
    root->capacity = 0;
    root->children = NULL;

    while ((nread = getline(&line, &linelen, stdin)) > 0) {
        if (line[nread - 1] == '\n')
            line[nread - 1] = '\0';

        size_t height = path_add(root, line);

        if (height > tree_height)
            tree_height = height;
    }

    // If all paths have a common root component, we draw that as the
    // tree's root. Not if we only have a single lonely path -- that would
    // just echo that path.
    if (root->childcount == 1 && root->children[0]->childcount) {
        root = root->children[0];
        puts(root->name);
    }

    size_t *toddlers = malloc(tree_height * sizeof(size_t));
    print_tree(root, toddlers, 0);
}
