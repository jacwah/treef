#!/bin/bash

# Define environment variables to use alternative tools.
# Run as `VALGRIND= ./test.sh` if you don't want to use Valgrind.
: ${TREEF:=./treef}
: ${DIFF:=diff}
: ${TEST:=*}

if [ $(which valgrind) ]
then
    : ${VALGRIND=valgrind --tool=memcheck --leak-check=no --quiet}
fi

RED=$(tput setaf 1 2>/dev/null)
GREEN=$(tput setaf 2 2>/dev/null)
RESET=$(tput sgr0 2>/dev/null)

OK="${GREEN}✓${RESET}"
FAIL="${RED}✗${RESET}"

successful=0
failed=0

for infile in tests/${TEST}.in
do
    path=${infile%.in}
    name=${path##*/}

    diff=$($DIFF --text ${path}.out <($VALGRIND $TREEF < "$infile"))

    if [ -z "$diff" ]
    then
        let successful+=1
        echo "$OK $name"
    else
        let failed+=1
        echo "$FAIL $name"
        echo "$diff"
    fi
done

[[ $successful > 0 ]] && SUCCESS_COLOR="$GREEN" || SUCCESS_COLOR="$RED"
[[ $failed > 0 ]] && FAIL_COLOR="$RED" || FAIL_COLOR="$GREEN"

echo
echo "${SUCCESS_COLOR}${successful}${RESET} successful, ${FAIL_COLOR}${failed}${RESET} failed."

exit $((failed > 0))
