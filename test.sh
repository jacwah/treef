#!/bin/bash

TREEF="./treef"
DIFF="diff"

RED=$(tput setaf 1 2>/dev/null)
GREEN=$(tput setaf 2 2>/dev/null)
RESET=$(tput sgr0 2>/dev/null)

OK="${GREEN}✓${RESET}"
FAIL="${RED}✗${RESET}"

successful=0
failed=0

for infile in tests/*.in
do
    path=${infile%.in}
    name=${path##*/}

    diff=$($DIFF ${path}.out <($TREEF < "$infile"))

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
