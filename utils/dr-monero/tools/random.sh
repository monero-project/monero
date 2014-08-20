
#!/bin/bash

# $RANDOM returns a different random integer at each invocation.
# Nominal range: 0 - 32767 (signed 16-bit integer).

MAXCOUNT=50
count=1

#echo
#echo "$MAXCOUNT random numbers:"
#echo "-----------------"
while [ "$count" -le $MAXCOUNT ]      # Generate 10 ($MAXCOUNT) random integers.
do
  number=$RANDOM
#  echo $number
  let "count += 1"  # Increment count.
done
#echo "-----------------"

# If you need a random int within a certain range, use the 'modulo' operator.
# This returns the remainder of a division operation.

RANGE=500

#echo

number=$RANDOM
let "number %= $RANGE"
#           ^^
#echo "Random number less than $RANGE  ---  $number"

#echo



#  If you need a random integer greater than a lower bound,
#+ then set up a test to discard all numbers below that.

FLOOR=200

number=0   #initialize
while [ "$number" -le $FLOOR ]
do
  number=$RANDOM
done
#echo "Random number greater than $FLOOR ---  $number"
#echo

   # Let's examine a simple alternative to the above loop, namely
   #       let "number = $RANDOM + $FLOOR"
   # That would eliminate the while-loop and run faster.
   # But, there might be a problem with that. What is it?



# Combine above two techniques to retrieve random number between two limits.
number=0   #initialize
while [ "$number" -le $FLOOR ]
do
  number=$RANDOM
  let "number %= $RANGE"  # Scales $number down within $RANGE.
done
#echo "Random number between $FLOOR and $RANGE ---  $number"
#echo



# Generate binary choice, that is, "true" or "false" value.
BINARY=2
T=1
number=$RANDOM

let "number %= $BINARY"
#  Note that    let "number >>= 14"    gives a better random distribution
#+ (right shifts out everything except last binary digit).
#if [ "$number" -eq $T ]
#then
#  echo "TRUE"
#else
#  echo "FALSE"
#fi  

#echo


# Generate a toss of the dice.
SPOTS=6   # Modulo 6 gives range 0 - 5.
          # Incrementing by 1 gives desired range of 1 - 6.
          # Thanks, Paulo Marcel Coelho Aragao, for the simplification.
die1=0
die2=0
# Would it be better to just set SPOTS=7 and not add 1? Why or why not?

# Tosses each die separately, and so gives correct odds.

    let "die1 = $RANDOM % $SPOTS +1" # Roll first one.
    let "die2 = $RANDOM % $SPOTS +1" # Roll second one.
    #  Which arithmetic operation, above, has greater precedence --
    #+ modulo (%) or addition (+)?


let "throw = $die1 + $die2"
echo $throw


exit 0
