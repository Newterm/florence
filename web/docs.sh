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
	rsvg -x 0.6 -y 0.6 svg/docs-florence-$2.svg $output

	php svg/key.php reduce > svg/minimize.svg
	rsvg svg/minimize.svg $1/minimize.png
	php svg/key.php config > svg/configuration.svg
	rsvg svg/configuration.svg $1/configuration.png
	php svg/key.php move > svg/move.svg
	rsvg svg/move.svg $1/move.png
	php svg/key.php bigger > svg/bigger.svg
	rsvg svg/bigger.svg $1/bigger.png
	php svg/key.php smaller > svg/smaller.svg
	rsvg svg/smaller.svg $1/smaller.png
	php svg/switch.php > svg/switch-$2.svg
	rsvg svg/switch-$2.svg $1/switch.png
	rsvg svg/close.svg $1/close.png
}

setxkbmap -print >/tmp/xkbmap.tmp
make_docs_svg ../docs/C/figures us C
make_docs_svg ../docs/ru/figures ru ru_RU.UTF8
make_docs_svg ../docs/fr/figures fr fr_FR.UTF8
cat /tmp/xkbmap.tmp | xkbcomp - $DISPLAY 2>/dev/null && rm /tmp/xkbmap.tmp

