#!/bin/bash

# ./applyBM.sh BASEPATH [filelist]
# the optional filelist argument can be the name of a
# file that supplies a list of names to be converted

programPath="../simulator/build/bem-model"
model="bem-agi"
aero="4.797071 4.168863 5.89"   # cl cd k; empty = use compile-time defaults

if [ "$1" != "" ]
then
  echo "Using Directory "$1
fi

mkdir -p "$1""$model"

if [ -e "$2" ] 
then
  while read l
  do
    fname="merged_""$l"".csv"
    dname=$1
    newfile=${fname:7}
    echo "$dname" "$fname" "$newfile"
    eval "$programPath" "$dname""$fname" "$dname""/""$model""/""$model""_""$newfile" "$aero"
  done < "$2"
else
  for csv in "$1"merged_*seg*.csv
  do
    fname=$(basename $csv)
    dname=$(dirname $csv)
    newfile=${fname:7}
    # echo "$dname" "$fname" "$newfile"
    eval "$programPath" "$csv" "$dname""/""$model""/""$model""_""$newfile" "$aero"
  done
fi

