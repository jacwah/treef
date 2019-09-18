#include "color.h"
#include "tree.h"
#include "nstr.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

static struct nstr_block *g_nstrbp;
static struct nstr *g_type_sgr[TYPE_COUNT];
static struct nstr g_empty_string;
static struct nstr *g_sgr0_eol;
static bool g_enabled;

enum type
get_type(const char *path)
{
    if (g_enabled) {
        struct stat st;
        if (lstat(path, &st)) {
            return TYPE_NONE;
        } else {
            // TODO: multihardlink (mh)
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
                    // TODO: orphan
                    return TYPE_LINK;
                case S_IFSOCK:
                    return TYPE_SOCK;
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
    // TODO: fallback to superset when sgr not defined (eg suid->exec)
    while (*colors) {
        enum type type;
#define id(a,b) ((a<<8)|b)
        switch (id(colors[0],colors[1])) {
            case id('f','i'): type = TYPE_FILE;   break;
            case id('d','i'): type = TYPE_DIR;    break;
            case id('l','n'): type = TYPE_LINK;   break;
            case id('p','i'): type = TYPE_PIPE;   break;
            case id('s','o'): type = TYPE_SOCK;   break;
            case id('b','d'): type = TYPE_BLOCK;  break;
            case id('c','d'): type = TYPE_CHAR;   break;
            case id('o','r'): type = TYPE_ORPHAN; break;
            case id('e','x'): type = TYPE_EXEC;   break;
            case id('s','u'): type = TYPE_SUID;   break;
            case id('s','g'): type = TYPE_SGID;   break;
            case id('s','t'): type = TYPE_STICKY; break;
            case id('t','w'): type = TYPE_OWRITS; break;
            case id('o','w'): type = TYPE_OWRITE; break;
            case id('*','.'): // TODO support filename wildcards
            default:
                //fprintf(stderr, "unrecognized: %.2s\n", colors);
                colors += 2;
                while (*colors && *colors++ != ':');
                continue;
        }
#undef id
        colors += 2;

        if (*colors++ != '=')
            goto fail;

        size_t len = 0;
        while (colors[len] && colors[len] != ':') {
            if (!isdigit(colors[len]) && colors[len] != ';')
                goto fail;
            len++;
        }

        struct nstr *sgr = nstr_alloc(g_nstrbp, (nstrlen)(2+len+1));
        sgr->n = (nstrlen)(2+len+1);

        sgr->str[0] = '\x1b';
        sgr->str[1] = '[';

        memcpy(2+sgr->str, colors, (nstrlen)len);
        sgr->str[2+len] = 'm';

        g_type_sgr[type] = sgr;
        colors += len;

        while (*colors == ':') colors++;
    }

    return true;

fail:
    fprintf(stderr, "warning: failed to parse LS_COLORS\n");
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
        g_type_sgr[TYPE_DIR]    = bsd_sgr(colors[0],  colors[1]);
        g_type_sgr[TYPE_LINK]   = bsd_sgr(colors[2],  colors[3]);
        g_type_sgr[TYPE_SOCK]   = bsd_sgr(colors[4],  colors[5]);
        g_type_sgr[TYPE_PIPE]   = bsd_sgr(colors[6],  colors[7]);
        g_type_sgr[TYPE_EXEC]   = bsd_sgr(colors[8],  colors[9]);
        g_type_sgr[TYPE_BLOCK]  = bsd_sgr(colors[10], colors[11]);
        g_type_sgr[TYPE_CHAR]   = bsd_sgr(colors[12], colors[13]);
        g_type_sgr[TYPE_SUID]   = bsd_sgr(colors[14], colors[15]);
        g_type_sgr[TYPE_SGID]   = bsd_sgr(colors[16], colors[17]);
        g_type_sgr[TYPE_OWRITS] = bsd_sgr(colors[18], colors[19]);
        g_type_sgr[TYPE_OWRITE] = bsd_sgr(colors[20], colors[21]);

        g_type_sgr[TYPE_ORPHAN] = g_type_sgr[TYPE_LINK];
        g_type_sgr[TYPE_STICKY] = g_type_sgr[TYPE_DIR];
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
        g_enabled = true;
    else if ((env = getenv("LSCOLORS")) && *env && parse_bsd_colors(env))
        g_enabled = true;
    else if (getenv("CLICOLOR"))
        g_enabled = set_default_bsd_colors();
#if 0
    else if (getenv("TERMCOLOR"))
        g_enabled = set_default_gnu_colors;
#endif

    return g_enabled;
}

void
print_node(struct node node)
{
    if (g_enabled) {
        bool colored = g_type_sgr[node.type]->n;
        if (colored) fwrite(g_type_sgr[node.type]->str, 1, g_type_sgr[node.type]->n, stdout);
        fwrite(node.name->str, 1, node.name->n, stdout);
        if (colored) fwrite(g_sgr0_eol->str, 1, g_sgr0_eol->n, stdout);
        else putc('\n', stdout);
    } else {
        fwrite(node.name->str, 1, node.name->n, stdout);
        putc('\n', stdout);
    }
}
