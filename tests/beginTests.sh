#!/bin/bash

TG=$1
LOGFILE="/tmp/$TG.log"

NAME=$(cat groups/$TG/config.json | jq .name)
DESCRIPTION=$(cat groups/$TG/config.json | jq .description)

rm $LOGFILE
touch $LOGFILE
echo "Beginning test group $NAME" >> $LOGFILE
echo $DESCRIPTION >> $LOGFILE