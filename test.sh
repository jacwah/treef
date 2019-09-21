#!/bin/sh

# Define environment variables to use alternative tools.
# Run as `VALGRIND= ./test.sh` if you don't want to use Valgrind.
: ${TREEF:=./treef}
: ${DIFF:=diff -au}
: ${TEST:=*}

if [ $(which valgrind) ]
then
    : ${VALGRIND=valgrind --tool=memcheck --leak-check=no --quiet}
fi

# I try to support FreeBSD here by falling back to termcap names.
RED=$(tput setaf 1 2>/dev/null || tput AF 1 2>/dev/null)
GREEN=$(tput setaf 2 2>/dev/null || tput AF 2 2>/dev/null)
RESET=$(tput sgr0 2>/dev/null || tput me 2>/dev/null)

OK="${GREEN}✓${RESET}"
FAIL="${RED}✗${RESET}"

successful=0
failed=0

for infile in tests/${TEST}.in
do
    path=${infile%.in}
    name=${path##*/}

    $VALGRIND $TREEF <${infile} | $DIFF ${path}.out -

    if [ $? -eq 0 ]
    then
        successful=`expr $successful + 1`
        echo "$OK $name"
    else
        failed=`expr $failed + 1`
        echo "$FAIL $name"
    fi
done

test $successful > 0 && SUCCESS_COLOR="$GREEN" || SUCCESS_COLOR="$RED"
test $failed > 0 && FAIL_COLOR="$RED" || FAIL_COLOR="$GREEN"

echo
echo "${SUCCESS_COLOR}${successful}${RESET} successful, ${FAIL_COLOR}${failed}${RESET} failed."

exit $((failed > 0))
