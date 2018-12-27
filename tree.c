#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "tree.h"
#include "color.h"

static void
prepare_add(struct tree *tree)
{
    if (tree->capacity == tree->count) {
        if (tree->capacity == 0)
            tree->capacity = 8;
        else
            tree->capacity *= 2;

        tree->nodes = realloc(
                tree->nodes,
                sizeof(struct node) * tree->capacity);
    }
}

static int
add(struct tree *tree, int parent_idx, int last_sibling_idx, struct nstr *name, enum type type)
{
    prepare_add(tree);

    struct node *new = &tree->nodes[tree->count];
    new->name = name;
    new->sibling_idx = -1;
    new->child_idx = -1;
    new->type = type;

    // Link node into the tree
    if (last_sibling_idx >= 0)
        tree->nodes[last_sibling_idx].sibling_idx = (int) tree->count;
    else if (parent_idx >= 0)
        tree->nodes[parent_idx].child_idx = (int) tree->count;

    return (int) tree->count++;
}

struct find_result {
    int node_idx;
    int last_sibling_idx;
};

static struct find_result
find(struct tree *tree, int parent_idx, char *name, int len)
{
    struct find_result result;
    result.last_sibling_idx = -1;

    if (tree->count) {
        int sibling_idx = -1;

        if (parent_idx >= 0)
            sibling_idx = tree->nodes[parent_idx].child_idx;
        else
            sibling_idx = 0;

        while (sibling_idx >= 0) {
            // If this node already exists, no need to add it again
            struct nstr *sibling_name = tree->nodes[sibling_idx].name;
            if (sibling_name->n == len && !memcmp(sibling_name->str, name, (size_t)len)) {
                result.node_idx = sibling_idx;
                return result;
            }

            result.last_sibling_idx = sibling_idx;
            sibling_idx = tree->nodes[sibling_idx].sibling_idx;
        }
    }

    result.node_idx = -1;
    return result;
}

static int
find_or_add(struct tree *tree, struct nstr_block *nstrbp, int parent_idx, char *name, int len, enum type type)
{
    struct find_result found = find(tree, parent_idx, name, len);
    if (found.node_idx >= 0)
        return found.node_idx;
    else
        return add(tree, parent_idx, found.last_sibling_idx, nstr_dup(nstrbp, len, name), type);
}

void
tree_add_path(struct tree *tree, struct nstr_block *nstrbp, int parent_idx, char *path, int off, enum type type)
{
    size_t height = 0;

    for (;;) {
        while (path[off] == '/') off++;

        // Occurs on trailing slash
        if (path[off] == '\0')
            break;

        ++height;
        char *slash = strchr(path+off, '/');

        if (!slash) {
            // Leaf node
            // TODO strchr with len
            find_or_add(tree, nstrbp, parent_idx, path+off, (int)strlen(path+off), type);
            break;
        } else {
            int grandparent_idx = parent_idx;
            int len = (int)(slash-path)-off;
            struct find_result found_parent = find(tree, grandparent_idx, path+off, len);

            if (found_parent.node_idx >= 0) {
                parent_idx = found_parent.node_idx;
            } else {
                *slash = '\0';
                enum type parent_type = get_type(path);
                *slash = '/';
                struct nstr *parent_name = nstr_dup(nstrbp, len, path+off);
                parent_idx = add(tree, grandparent_idx, found_parent.last_sibling_idx, parent_name, parent_type);
            }

            off += len+1;
        }
    }

    if (height > tree->height)
        tree->height = height;
}

static void
print_nodes(struct tree *tree, int *tree_path, int depth, int node_idx)
{
    for (;;) {
        struct node node = tree->nodes[node_idx];
        int child_idx = node.child_idx;
        int next_idx = node.sibling_idx;

        for (int anscestor = 0; anscestor < depth; anscestor++) {
            if (tree->nodes[tree_path[anscestor]].sibling_idx < 0)
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
                next_idx = tree->nodes[node_idx].sibling_idx;
            }
            if (next_idx < 0)
                break;
            node_idx = next_idx;
        }
    }
}

void
print_tree(struct tree *tree)
{
    int *path = malloc(sizeof(int) * tree->height);
    int root_idx;

    // If all paths have a common root component, we draw that as the
    // tree's root. Not if we only have a single lonely path -- that would
    // just echo that path.
    if (tree->count > 1 && tree->nodes[0].sibling_idx < 0) {
        print_node(tree->nodes[0]);
        root_idx = 1;
    } else {
        root_idx = 0;
    }

    print_nodes(tree, path, 0, root_idx);
}

