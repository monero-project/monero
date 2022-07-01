files=monero*.[1-8]

for f in $files
do
    section=${f##*.}
    mkdir /usr/local/man/man$section
    out=/usr/local/man/man$section/$(basename $f)
    echo  $f "** copying to MANPATH"
    cp $f $out
    gzip $out
done
