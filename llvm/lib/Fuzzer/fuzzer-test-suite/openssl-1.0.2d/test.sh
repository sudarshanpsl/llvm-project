#!/bin/bash
set -x
SCRIPT_DIR=$(dirname $0)
EXECUTABLE_NAME_BASE=$(basename $SCRIPT_DIR)
CORPUS=CORPUS-$EXECUTABLE_NAME_BASE
JOBS=8
rm -rf $CORPUS
mkdir $CORPUS
[ -e $EXECUTABLE_NAME_BASE ] && ./$EXECUTABLE_NAME_BASE -artifact_prefix=$CORPUS/ -max_len=512  -jobs=$JOBS -workers=$JOBS $CORPUS
grep 'Assertion `strcmp(openssl_results.exptmod, gcrypt_results.exptmod)==0. failed.' fuzz-0.log
