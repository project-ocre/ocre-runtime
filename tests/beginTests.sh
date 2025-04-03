#!/bin/bash

TG=$1
LOGFILE="/tmp/$TG.log"

clStr() {
    ARG=$1
    echo "${ARG:1:-1}"
}

CONF=groups/$TG/config.json
NAME=$(clStr "$(cat $CONF | jq .name)")
DESCRIPTION=$(clStr "$(cat $CONF | jq .description)")

rm $LOGFILE > /dev/null 2>&1
touch $LOGFILE
echo "Beginning test group $NAME" >> $LOGFILE
echo $DESCRIPTION >> $LOGFILE
echo >> $LOGFILE

parseJSON() {
    echo $(cat $CONF | jq .$1)
}

parseJSON2() {
    echo $(echo $1 | jq .$2)
}

parseJSONArray() {
    echo $(cat $CONF | jq -r ".$1[] | @base64")
}

parseJSONArray2() {
    echo $(echo $1 | jq -r ".$2[] | @base64")
}

printHeader() {
    LEN=${#1}
    FORMAT=""
    for ((i = 1; i <= $LEN; i ++)); do
        FORMAT+="="
    done

    echo $1 >> $LOGFILE
    echo $FORMAT >> $LOGFILE
    echo >> $LOGFILE
}

CWD=$(pwd)

# Setup
echo "Entering Setup..." >> $LOGFILE
echo >> $LOGFILE
for RAW_ROW in $(parseJSONArray "setup"); do
    ROW=$(echo $RAW_ROW | base64 -di)
    printHeader "Script: $(clStr "$(parseJSON2 "$ROW" "name")")"
    cd groups/$TG
    bash -c "bash -c $(parseJSON2 "$ROW" "exec")" &>> $LOGFILE
    cd $CWD
    echo >> $LOGFILE
done

# Tests
echo "Entering Testing..." >> $LOGFILE
echo >> $LOGFILE
for RAW_ROW in $(parseJSONArray "tests"); do
    ROW=$(echo $RAW_ROW | base64 -di)

    if [[ "$(parseJSON2 "$ROW" "exec")" != "null" ]]; then
        printHeader "Test: $(clStr "$(parseJSON2 "$ROW" "name")")" >> $LOGFILE
        cd groups/$TG
        bash -c "bash -c $(parseJSON2 "$ROW" "exec")" &>> $LOGFILE
        cd $CWD
        echo >> $LOGFILE
    elif [[ "$(parseJSON2 "$ROW" "tests")" != "null" ]]; then
        printHeader "Test Group: $(clStr "$(parseJSON2 "$ROW" "name")")" >> $LOGFILE
        for RAW_TEST_ROW in $(parseJSONArray2 "$ROW" "tests"); do
            TEST_ROW=$(echo $RAW_TEST_ROW | base64 -di)

            if [[ "$(parseJSON2 "$TEST_ROW" "exec")" != "null" ]]; then
                printHeader "Test: $(clStr "$(parseJSON2 "$TEST_ROW" "name")")" >> $LOGFILE
                cd groups/$TG
                bash -c "bash -c $(parseJSON2 "$TEST_ROW" "exec")" &>> $LOGFILE
                cd $CWD
                echo >> $LOGFILE
            else
                echo "Could not figure out what to do with test block $(parseJSON2 "$TEST_ROW" "name")" >> $LOGFILE
                exit 1
            fi
        done
    else
        echo "Could not figure out what to do with test block $(parseJSON2 "$ROW" "name")" >> $LOGFILE
        exit 1
    fi
done

# Setup
echo "Entering Cleanup..." >> $LOGFILE
echo >> $LOGFILE
for RAW_ROW in $(parseJSONArray "cleanup"); do
    ROW=$(echo $RAW_ROW | base64 -di)
    printHeader "Script: $(clStr "$(parseJSON2 "$ROW" "name")")"
    cd groups/$TG
    bash -c "bash -c $(parseJSON2 "$ROW" "exec")" &>> $LOGFILE
    cd $CWD
    echo >> $LOGFILE
done

echo "Test suite is complete" >> $LOGFILE