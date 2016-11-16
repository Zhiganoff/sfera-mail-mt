#!/bin/bash

if [ -z "$1" ] ; then
    echo "Usage ./run_tests.sh <command>"
    exit 1
fi 

for i in {1..6} ; do 
    echo -n "Running test $i: "
    "$1" < tests/$i.sh > tests/$i.run.out 2> tests/$i.run.stderr

    if [ "$i" == 5 ] ; then
        grep -q 'exited: 71' tests/$i.run.stderr && \
        grep -q 'exited: 0' tests/$i.run.stderr && \
        diff tests/$i.out tests/$i.run.out > /dev/null && echo SUCCESS || echo FAIL
    else
        diff tests/$i.out tests/$i.run.out > /dev/null && echo SUCCESS || echo FAIL
    fi
done
