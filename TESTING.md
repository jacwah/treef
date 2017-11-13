# Testing treef
treef uses a simple end-to-end test system. Test cases are defined by
complementary `test_name.in` and `test_name.out` files in the `tests/`
directory. Infiles specify input to treef and outfiles the expected output. Take
a look at `tests/basic.in` and `tests/basic.out` for an example.

Tests are run using

    make test

which simply invokes the `test.sh` script. `test.sh` scans the `tests/`
directory for infiles and outfiles and runs them through `./treef`. Any
discrepancies it encounters are output as a diff.

## Writing test cases
Creating new test cases is easy with

    ./gen-test.sh test_name1 test_name2 ...

The test generator will open up an editor for each infile and then an outfile
prefilled to pass using the current `./treef`. Not saving the infile will cancel
the operation.
