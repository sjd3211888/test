#!/bin/bash
# define style
STYLEARG="-style=file"

# run clang-format -i on cache codes
clang_format_file()
{
    file="${1}"
    if [ ! -z "${STYLEARG}" ]; then
        echo "clang-format ${file}"
        #1.only clang-format git diff line in cache
        # generate a diff that only includes changes in ${file}
        GIT_DIFF=$(git diff --cached  -U0 --no-color ${file})
        # echo "GIT_DIFF: ${GIT_DIFF}"
        # apply clang-format to the diff
        FORMATTED_DIFF=$(echo "${GIT_DIFF}" | clang-format-diff -p 1)
        # echo "FORMATTED_DIFF: ${FORMATTED_DIFF}"
        if [ ! -z "${FORMATTED_DIFF}" ]; then
            # apply the formatted diff
            dirname=$(dirname "${file}")
            cd "${dirname}"
            echo "${FORMATTED_DIFF}" | git apply
            cd -
            if [ $? -eq 0 ]; then
                echo "apply command succeeded"
            else
                echo "apply command failed"
            fi
        fi

        # #2.clang-format all files in cache
        # clang-format -i ${STYLEARG} ${1}
        git add ${1}
    fi
}

# find all files in cache
case "${1}" in
    --about )
            echo "Runs clang-format on source files"
        ;;

    * )
        for file in `git diff-index --cached --name-only HEAD | grep -E '\.(cpp|hpp|cc|hh|c|h)$'`; do
            clang_format_file "${file}"
        done
        ;;
esac
