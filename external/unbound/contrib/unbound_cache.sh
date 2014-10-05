#!/sbin/sh
#
# --------------------------------------------------------------
# -- DNS cache save/load script
# --
# -- Version 1.0
# -- By Yuri Voinov (c) 2006, 2014
# --------------------------------------------------------------
#
# ident   "@(#)unbound_cache.sh     1.1     14/04/26 YV"
#

#############
# Variables #
#############

# Installation base dir
CONF="/etc/opt/csw/unbound"
BASE="/opt/csw"

# Unbound binaries
UC="$BASE/sbin/unbound-control"
FNAME="unbound_cache.dmp"

# OS utilities
BASENAME=`which basename`
CAT=`which cat`
CUT=`which cut`
ECHO=`which echo`
GETOPT=`which getopt`
ID=`which id`
PRINTF=`which printf`

###############
# Subroutines #
###############

usage_note ()
{
# Script usage note
 $ECHO "Usage: `$BASENAME $0` [-s] or [-l] or [-r] or [-h]"
 $ECHO
 $ECHO "l - Load - default mode. Warming up Unbound DNS cache from saved file. cache-ttl must be high value."
 $ECHO "s - Save - save Unbound DNS cache contents to plain file with domain names."
 $ECHO "r - Reload - reloadind new cache entries and refresh existing cache"
 $ECHO "h - this screen."
 $ECHO "Note: Run without any arguments will be in default mode."
 $ECHO "      Also, unbound-control must be configured."
 exit 0
}

root_check ()
{
 if [ ! `$ID | $CUT -f1 -d" "` = "uid=0(root)" ]; then
  $ECHO "ERROR: You must be super-user to run this script."
  exit 1
 fi
}

check_uc ()
{
 if [ ! -f "$UC" ]; then
  $ECHO .
  $ECHO "ERROR: $UC not found. Exiting..."
  exit 1
 fi
}

check_saved_file ()
{
 if [ ! -f "$CONF/$FNAME" ]; then
  $ECHO .
  $ECHO "ERROR: File $CONF/$FNAME does not exists. Save it first."
  exit 1
 fi
}

save_cache ()
{
 # Save unbound cache
 $PRINTF "Saving cache in $CONF/$FNAME..."
 $UC dump_cache>$CONF/$FNAME
 $ECHO "ok"
}

load_cache ()
{
 # Load saved cache contents and warmup DNS cache
 $PRINTF "Loading cache from saved $CONF/$FNAME..."
 check_saved_file
 $CAT $CONF/$FNAME|$UC load_cache
}

reload_cache ()
{
 # Reloading and refresh existing cache and saved dump
 save_cache
 load_cache
}

##############
# Main block #
##############

# Root check
root_check

# Check unbound-control
check_uc

# Check command-line arguments
if [ "x$1" = "x" ]; then
# If arguments list empty, load cache by default
 load_cache
else
 arg_list=$1
 # Parse command line
 set -- `$GETOPT sSlLrRhH: $arg_list` || {
  usage_note 1>&2
 }

  # Read arguments
 for i in $arg_list
  do
   case $i in
    -s | -S) save_cache;;
    -l | -L) load_cache;;
    -r | -R) reload_cache;;
    -h | -H | \?) usage_note;;
   esac
   break
  done
fi

exit 0