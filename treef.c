/* TODO or IDEA
 * Normalise paths e.g. consecutive '/'
 * Generalise to arbitrary delimiters
 * Trailing '/' = directory
 * Handle root anchored paths
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct node {
    char *name;
    size_t childcount;
    size_t capacity;
    struct node **children;
};

struct node *node_add(struct node *parent, char *name)
{
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

        parent->children = realloc(parent->children, sizeof(struct node *) * parent->capacity);
    }

    parent->children[parent->childcount++] = node;

    return node;
}

void path_add(struct node *root, char *path)
{
    // TEST: what if duplicates?
    char *sep = strchr(path, '/');

    if (!sep) {
        node_add(root, path);
    } else {
        for (int i = 0; i < root->childcount; i++) {
            if (!strncmp(root->children[i]->name, path, sep - path)) {
                path_add(root->children[i], sep + 1);
                return;
            }
        }

        // "abc/def": sep = path + 3 -> sep - path = 3 -> path[3] = '/'
        size_t len = sep - path;
        char *name = malloc(len + 1);
        memcpy(name, path, len);
        name[len] = '\0';

        struct node *parent = node_add(root, name);
        path_add(parent, sep + 1);
    }
}

void tree_print_debug(struct node *root, char *prefix)
{
    char *new_prefix;
    size_t prefix_len;
    size_t name_len;

    if (prefix)
        prefix_len = strlen(prefix);
    else
        prefix_len = 0;

    if (root->name)
        name_len = strlen(root->name);
    else
        name_len = 0;

    new_prefix = malloc(prefix_len + !!prefix_len + name_len + 1);

    memcpy(new_prefix, prefix, prefix_len);

    if (prefix_len)
        new_prefix[prefix_len] = '/';

    memcpy(new_prefix + prefix_len + !!prefix_len, root->name, name_len);
    new_prefix[prefix_len + !!prefix_len + name_len] = '\0';

    for (int i = 0; i < root->childcount; i++) {
        if (root->children[i]->childcount == 0) {
            printf("%s/%s\n", new_prefix, root->children[i]->name);
        } else {
            tree_print_debug(root->children[i], new_prefix);
        }
    }
}

int main(int argc, char **argv)
{
    char *line = NULL;
    size_t linelen = 0;
    size_t nread = 0;
    struct node *root = malloc(sizeof(struct node));

    root->name = "";
    root->childcount = 0;
    root->capacity = 0;
    root->children = NULL;

    while ((nread = getline(&line, &linelen, stdin)) != -1) {
        if (line[nread - 1] == '\n')
            line[nread - 1] = '\0';

        path_add(root, line);
    }

    tree_print_debug(root, NULL);
}
