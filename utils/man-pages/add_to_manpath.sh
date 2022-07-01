files=monero*.1

for f in $files
do
    out=/usr/local/man/man1/$(basename $f)
    echo  $f "** copying to MANPATH"
    cp $f $out
    gzip $out
done
