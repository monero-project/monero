#!/bin/bash -e

# maintainer (ask me any questions): rfree

if [[ ! -r "Doxyfile" ]] ; then
	echo "Error, can not read the Doxyfile - make sure to run this script from top of monero project, where the Doxyfile file is located"
	exit 1
fi

wwwdir="$HOME/venue-www/"
if [[ ! -w "$wwwdir" ]] ; then
	echo "Error, can not write into wwwdir=$wwwdir. It should be a directory readable/connected to your webserver, or a symlink to such directory"
	exit 1
fi

if [[ ! -d "$wwwdir/doc" ]] ; then
	echo "Creating subdirs"
	mkdir "$wwwdir/doc"
fi

echo "Generating:"
doxygen Doxyfile && echo "Backup previous version:" && rm -rf ~/venue-www-previous && mv "$wwwdir/doc" ~/venue-www-previous && cp -ar doc/ "$wwwdir/" && echo "Done, builded and copied to public - the doxygen docs" && echo "size:" && du -Dsh "$wwwdir/" && echo "files:" && find "$wwwdir/" | wc -l



