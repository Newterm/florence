#TODO: use a makefile instead

[ -z "$PREFIX" ] && export PREFIX=/usr
[ -z "$DATADIR" ] && export DATADIR=$PREFIX/share

function fatal {
	echo $1 >&2
	exit 1
}

function doc {
	echo "translating documentation for $2 .."
	[ -e $DATADIR/gnome/help/florence/$1/florence.xml ] ||
		fatal "Please install florence with documentation (--with-help configure parameter)"
	export LC_ALL=$3
	php config.php > config-$1.xsl
	php home.php > xhtml/$2.html
	mkdir xhtml/$2
	cd xhtml/$2
	xmlto -m ../../config-$1.xsl xhtml $DATADIR/gnome/help/florence/$1/florence.xml
	mkdir figures
	cp $DATADIR/gnome/help/florence/$1/figures/* figures
	cd ../..
}

rm -rf xhtml/*
cp style.css xhtml
cp index.php xhtml
cp -r images xhtml
cp $DATADIR/pixmaps/florence.svg xhtml/images
doc C english C
doc fr francais fr_FR

