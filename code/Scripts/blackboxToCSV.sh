#!/bin/bash

if [ "$1" != "" ]
then
  echo "Using Directory " $1
fi


for log in "$1"*.BFL
do
  if [ -e "$log" ]
  then
    csv="$1""$(basename "$log" .BFL)"".01.csv"
    if test -f "$csv"
    then
      echo "File "$csv" is already converted"
    else
      echo "Converting ""$log"
      blackbox_decode --unit-rotation rad/s --unit-acceleration m/s2 --unit-height m --debug "$log"
      echo "2,g/^\s*\a/d|w!"  | vim -e "$csv"
      echo "2,s/^\(\(\s*-\?\d\+\.\?\d*,\)\{39}\).*/\1/g|w!|q"  | vim -e "$csv"
    fi
  else
    echo "$log"" does not exist"
    break
  fi
done
