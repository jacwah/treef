# treef
The tree formatter.

`treef` reads paths from stdin and formats them as a tree, like the `tree`
utility. Many tools' output can be piped to `treef`.

    treef
    ├── unix style
    │   ├── simple
    │   └── composable
    ├── mit license
    └── written by jacob wahlgren

## Getting started
Use `make` to build and install treef.

    make
    make install    # may require sudo

Take a look at the cookbook for some uses!

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

    grep -v '^!_' tags | cut -f 2 | treef

## Testing
Run automated tests with

    make test

The tests will run with Valgrind's memcheck if installed. See TESTING.md.
