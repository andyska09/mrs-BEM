#!/bin/bash

main() {
  # settings:
  #    set number of training segments
  Nval=${3:-49}
  Ntest=1
  N=$[$Nval+$Ntest]


  if [ -z "$1" ]
  then
    echo "Supply datapath as first argument"
    return 0
  elif [ -z "$2" ]
  then
    echo "Specify model to be trained (bem, fit or none)"
    return 0
  else
    dataset="$2"
  fi

  

  seed=42
  allfiles=$(ls "$1""/""$dataset" | sort --random-source=<(get_seeded_random $seed) -R)
  trainfiles=$(echo $allfiles | tr " " "\n" | head --lines=-$N)
  tmpfiles=$(echo $allfiles | tr " " "\n" | tail --lines=$N)
  valfiles=$(echo $tmpfiles | tr " " "\n" | head --lines=-$Ntest)
  testfiles=$(echo $tmpfiles | tr " " "\n" | tail --lines=$Ntest)

  echo "Removing the following files..."
  find $dataset/train ! -name '.gitignore' -type f -print -exec rm -f {} +
  find $dataset/validation ! -name '.gitignore' -type f -print -exec rm -f {} +
  find $dataset/test ! -name '.gitignore' -type f -print -exec rm -f {} +

  echo "=========================="
  echo "Creating new data files..."
  echo "=========================="
  for file in $trainfiles
  do
    ln -f "$1""/""$dataset""/""$file" $dataset/train/
    echo $dataset/train/$file
  done

  for file in $valfiles
  do
    ln -f "$1""/""$dataset""/""$file" $dataset/validation/
    echo $dataset/validation/$file
  done
  
  for file in $testfiles
  do
    ln -f "$1""/""$dataset""/""$file" $dataset/test/
    echo $dataset/test/$file
  done
  
  if [ -e "testset.txt" ] 
  then
    mv $dataset/test/* $dataset/train/
    while read l
    do
      find -type f -name "$dataset""*""$l""*" -print -exec mv -t $dataset/test {} + 
    done < "testset.txt"
  fi

}


get_seeded_random()
{
  seed="$1"
  openssl enc -aes-256-ctr -pass pass:"$seed" -nosalt \
    </dev/zero 2>/dev/null
  }

main "$@"



