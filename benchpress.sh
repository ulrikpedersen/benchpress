#!/bin/bash

usage="$0 perform a scan of compression levels and threads

Usage: $0 [options] FILE DATASET

  FILE:    name of input data file in HDF5 format
  DATASET: name of dataset in FILE to use as input

  Options are:
   -h         print this help text
   -t         maximum number of threads in scan
"

black='\E[30m'
red='\E[31m'
green='\E[32m'
yellow='\E[33m'
blue='\E[34m'
magenta='\E[35m'
cyan='\E[36m'
white='\E[37m'

cecho ()
{
  local default_msg="No message passed."
  message=${1:-$default_msg}
  color=${2:-$white}

  echo -e "$color$message"
  #echo "$message"
  tput sgr0 # Reset to normal.
  return
}  


threads="1"
while getopts "ht:" opt
do
  case $opt in
	h) echo "$usage"; exit;;
    t) threads="${OPTARG}";;

    # Unknown option. No need for an error, getopts informs
    # the user itself.
    \?) exit 1;;
  esac
done
shift $(( $OPTIND -1 ))

if [ $# -lt 2 ]
  then
    cecho "### ERROR ### Not enough arguments." $red
    echo "$usage"
    exit
fi
input_file=$1
input_dset=$2

echo ${input_file}
echo ${input_dset}

for thread in `seq ${threads}`
do
  for level in `seq 0 9`
    do
      cmd="./Release/benchpress -l ${level} -t ${thread} -i 5 ${input_file} ${input_dset}"
      #echo ${cmd}
      ${cmd}
  done
done


