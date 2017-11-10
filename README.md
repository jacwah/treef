# treef
The tree formatter.

`treef` reads paths from stdin and formats them as a tree, like the `tree`
utility. Many tools' output can be piped to `treef`, allowing infinite
compositions.

    treef
    ├── UNIX philosophy
    │   ├── Simple
    │   └── Composable
    ├── MIT license
    │   └── Contributions welcome!
    └── Written by Jacob Wahlgren

## Cookbook
The output of a wide range of command line tools can be visualised with
`treef`. Here is a collection of recipes as inspiration, some more useful than
others.

**List files tracked by Git.** This is my savior in projects with `build/`,
`node_modules/` etc where `tree` is too cluttered. I have it aliased to `gt` in
bash for ergonomics. `ls-files` can also be used to show files in the index,
ignored files, modified files and more.

    git ls-files | treef

**Visualise your .gitignore.** Kind of weird with wildstars, but works all
right.

    treef < .gitignore

**Recursively list HTML files.** There are endless possibilities here. `find`
has loads of comparators like `-mtime`, `-size` and `-user`.

    find . -name '*.html' | treef

**Recursively list files containing a magic number.** With grep comes the whole
power of regular expressions.

    grep -lR 123 * | treef

**What files are referred to in a ctags file?** Just for fun :)

    grep -v '^!_' tags | cut -f 2 | sort | uniq | treef

