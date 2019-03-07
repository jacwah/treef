#include <sys/stat.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "color.h"
#include "tree.h"
#include "nstr.h"

static struct nstr_block *g_nstrbp;
static struct nstr *g_type_sgr[TYPE_COUNT];
static struct nstr g_empty_string;
static struct nstr *g_sgr0_eol;
static bool g_initted;

enum type
get_type(const char *path)
{
    if (g_initted) {
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

static bool
parse_gnu_colors(const char *colors)
{
    /* TODO */
    (void)colors;
    return false;
}

static struct nstr *
bsd_sgr(char f, char b)
{
    if (f == 'x' && b == 'x')
        return &g_empty_string;
    // ESC [ff;bb;am
    struct nstr *sgr = nstr_alloc(g_nstrbp, 10);

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
        g_type_sgr[TYPE_DIR]  = bsd_sgr(colors[0],  colors[1]);
        g_type_sgr[TYPE_LINK] = bsd_sgr(colors[2],  colors[3]);
        g_type_sgr[TYPE_SOCK] = bsd_sgr(colors[4],  colors[5]);
        g_type_sgr[TYPE_PIPE] = bsd_sgr(colors[6],  colors[7]);
        g_type_sgr[TYPE_EXEC] = bsd_sgr(colors[8],  colors[9]);
        g_type_sgr[TYPE_BLOCK]= bsd_sgr(colors[10], colors[11]);
        g_type_sgr[TYPE_CHAR] = bsd_sgr(colors[12], colors[13]);
        g_type_sgr[TYPE_SUID] = bsd_sgr(colors[14], colors[15]);
        g_type_sgr[TYPE_SGID] = bsd_sgr(colors[16], colors[17]);
        g_type_sgr[TYPE_OTHRS]= bsd_sgr(colors[18], colors[19]);
        g_type_sgr[TYPE_OTHR] = bsd_sgr(colors[20], colors[21]);
        return true;
    } else {
        return false;
    }
}

static bool
set_default_bsd_colors()
{
    return parse_bsd_colors("exfxcxdxbxegedabagacad");
}

bool
color_init(struct nstr_block *nstrbp)
{
    g_nstrbp = nstrbp;
    for (int i = 0; i < TYPE_COUNT; ++i)
        g_type_sgr[i] = &g_empty_string;

    g_sgr0_eol = nstr_alloc(g_nstrbp, 4);
    g_sgr0_eol->str[0] = '\x1b';
    g_sgr0_eol->str[1] = '[';
    g_sgr0_eol->str[2] = 'm';
    g_sgr0_eol->str[3] = '\n';
    g_sgr0_eol->n = 4;

    char *env;
    if ((env = getenv("LS_COLORS")) && *env && parse_gnu_colors(env))
        g_initted = true;
    else if ((env = getenv("LSCOLORS")) && *env && parse_bsd_colors(env))
        g_initted = true;
    else if (getenv("CLICOLOR"))
        g_initted = set_default_bsd_colors();
#if 0
    else if (getenv("TERMCOLOR"))
        g_initted = set_default_gnu_colors;
#endif

    return g_initted;
}

void
print_node(struct node node)
{
    if (g_initted) {
        fwrite(g_type_sgr[node.type]->str, 1, g_type_sgr[node.type]->n, stdout);
        fwrite(node.name->str, 1, node.name->n, stdout);
        fwrite(g_sgr0_eol->str, 1, g_sgr0_eol->n, stdout);
    } else {
        fwrite(node.name->str, 1, node.name->n, stdout);
        putc('\n', stdout);
    }
}
