#/bin/sh

output=$1
[ -z "$PREFIX" ] && export PREFIX=/usr
[ -z "$DATADIR" ] && export DATADIR=$PREFIX/share
[ -z "$output" ] && output=florence.png

python keymap.py > keymap.xml
xsltproc --xinclude style.xsl $DATADIR/florence/styles/default/florence.style >style.svg
xsltproc --xinclude florence.xsl $DATADIR/florence/layouts/florence.xml >florence.svg
rsvg-convert -x 0.6 -y 0.6 florence.svg -o $output

