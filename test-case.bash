#!/bin/bash

RED='\033[0;31m'
GREEN='\033[0;32m'
ORANGE='\033[0;33m'
NC='\033[0m'

TEST_FILES_DIR="."
ER_FILE_NAME="output.expected.bin"
INPUT_FILE_NAME="input.bin"
INPUT=${TEST_FILES_DIR}/${INPUT_FILE_NAME}
ER=${TEST_FILES_DIR}/${ER_FILE_NAME}
TMP_FILE="tmp.bin"
AR="output.bin"

bsort=/home/nikita/Programs/bsort/src/bsort

./test_gen.out $1

START_TIME="$(date -u +%s.%N)"
./ext_sort.out
END_TIME="$(date -u +%s.%N)"
./test.out
RC=$?
ELAPSED="$(bc <<<"$END_TIME-$START_TIME")"

# copy header with number of bytes
#dd if=${INPUT} of=${ER} count=1 bs=8
#dd if=${INPUT} of=${TMP_FILE} skip=1 bs=8
#${bsort} -k 8 -r 8 ${TMP_FILE}
#dd if=${TMP_FILE} of=${ER} seek=1 bs=8

if [[ !(-d ./tests) ]]; then
    mkdir tests
fi

#rm ${TMP_FILE}
#cmp ${AR} ${ER}
#RC=$(($? + $RC))

if [[ ${RC} -ne 0 ]]
then
    echo -e "[${RED} WA  ${NC}]\tfor ${1}"
    cp ./input.bin ./tests/input.bin.$(date +%N)
elif [[ $(bc <<<"$ELAPSED>1") -eq 1 ]]
then
    echo -e "[${ORANGE} TLE ${NC}]\t ${ELAPSED}s for ${1}"
else
    echo -e "[${GREEN} OK  ${NC}]\t ${ELAPSED}s for ${1}"
fi