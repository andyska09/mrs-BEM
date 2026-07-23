#!/bin/bash

# ./applyBM.sh BASEPATH MODEL CONFIG [filelist]
#   MODEL    output subfolder name (e.g. a CMA-ES run timestamp)
#   CONFIG   path to the params yaml (e.g. CMAES-results/<ts>/best.yaml)
# the optional filelist argument can be the name of a
# file that supplies a list of names to be converted

programPath="../simulator/build/bem-model"

if [ -z "$1" ] || [ -z "$2" ] || [ -z "$3" ]
then
  echo "usage: ./applyBM.sh BASEPATH MODEL CONFIG [filelist]"
  exit 1
fi

basepath="$1"
model="$2"
config="$3"
filelist="$4"

echo "Using Directory "$basepath
echo "Model "$model" Config "$config

mkdir -p "$basepath""$model"

if [ -e "$filelist" ]
then
  while read l
  do
    fname="merged_""$l"".csv"
    newfile=${fname:7}
    echo "$basepath" "$fname" "$newfile"
    eval "$programPath" "$basepath""$fname" "$basepath""/""$model""/""$model""_""$newfile" "$config"
  done < "$filelist"
else
  for csv in "$basepath"merged_*seg*.csv
  do
    fname=$(basename $csv)
    dname=$(dirname $csv)
    newfile=${fname:7}
    eval "$programPath" "$csv" "$dname""/""$model""/""$model""_""$newfile" "$config"
  done
fi
