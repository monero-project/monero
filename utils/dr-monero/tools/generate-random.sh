#!/bin/bash 


if [ "$#" -ne "2" ] ; then 
    echo "Usage: ./generate-random.sh numers-of-random-numbers output-file"  
    exit -1
fi



x=1; 
y=$1 
file=$2 

#set -x 
while [ $x -le $y ] ; do
    #./random.sh >> $file 
    echo "$(./random.sh)  $(./random.sh)" >> $file
    x=$[x + 1]
done
