#!/bin/bash
# This is a utility for creating test cases.

: ${TREEF:=./treef}
: ${EDITOR:=vi}

function die() {
    echo "error: $@"
    exit 1
}

if [[ $# -lt 1 ]]
then
    echo "usage: $0 TEST_NAME..."
    exit 1
fi

[ -e ${TREEF} ] || die "$TREEF doesn't exist, have you compiled?"

while [ "$#" -gt 0 ]
do
    infile="tests/$1.in"
    outfile="tests/$1.out"

    [ -e $infile ] && die "file $infile already exists"
    [ -e $outfile ] && die "file $outfile already exists"

    $EDITOR $infile

    if [ $? -eq 0 ] && [ -r $infile ]
    then
        $TREEF < $infile > $outfile
        $EDITOR $outfile
    else
        die $'no infile\nWrite test input and then save.'
    fi

    shift
done
