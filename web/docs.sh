#/bin/sh

function make_docs_svg {
	output=$1
	[ -z "$PREFIX" ] && export PREFIX=/usr
	[ -z "$DATADIR" ] && export DATADIR=$PREFIX/share
	output=$1/florence.png

	export LC_ALL=$3
	export LANGUAGE=$3
	setxkbmap $2
	php svg/docs.php > svg/docs-$2.xsl

	python svg/keymap.py > svg/keymap.xml
	xsltproc --xinclude svg/style.xsl $DATADIR/florence/styles/default/florence.style >svg/style.svg
	xsltproc --xinclude svg/florence.xsl $DATADIR/florence/layouts/florence.xml >svg/florence.svg

	xsltproc --xinclude svg/docs-$2.xsl svg/florence.svg >svg/docs-florence-$2.svg
	rsvg-convert -x 0.6 -y 0.6 svg/docs-florence-$2.svg -o $output

	php svg/key.php reduce > svg/minimize.svg
	rsvg-convert svg/minimize.svg -o $1/minimize.png
	php svg/key.php config > svg/configuration.svg
	rsvg-convert svg/configuration.svg -o $1/configuration.png
	php svg/key.php move > svg/move.svg
	rsvg-convert svg/move.svg -o $1/move.png
	php svg/key.php bigger > svg/bigger.svg
	rsvg-convert svg/bigger.svg -o $1/bigger.png
	php svg/key.php smaller > svg/smaller.svg
	rsvg-convert svg/smaller.svg -o $1/smaller.png
	php svg/switch.php > svg/switch-$2.svg
	rsvg-convert svg/switch-$2.svg -o $1/switch.png
	rsvg-convert svg/close.svg -o $1/close.png
}

setxkbmap -print >/tmp/xkbmap.tmp
make_docs_svg ../docs/C/figures us C
make_docs_svg ../docs/ru/figures ru ru_RU.UTF8
make_docs_svg ../docs/fr/figures fr fr_FR.UTF8
cat /tmp/xkbmap.tmp | xkbcomp - $DISPLAY 2>/dev/null && rm /tmp/xkbmap.tmp

