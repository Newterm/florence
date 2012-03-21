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
	export LANGUAGE=$3
	php config.php > config-$1.xsl
	php home.php > xhtml/$2.html
	mkdir xhtml/$2
	cd xhtml/$2
	xmlto -m ../../config-$1.xsl xhtml $DATADIR/gnome/help/florence/$1/florence.xml
	mkdir figures
	cp $DATADIR/gnome/help/florence/$1/figures/* figures
	cd ../../svg
	setxkbmap $4
	./svg.sh florence-$4.png
	cp florence-$4.png ../xhtml/images
	cd ..
}

xgettext home.php config.php tools.php svg/docs.php svg/switch.php -o messages.pot --from-code=utf-8 -l PHP
LINGUAS="fr_FR ru_RU"
for lang in $LINGUAS
do
	msgmerge -U locale/$lang/LC_MESSAGES/messages.po messages.pot
	cd locale/$lang/LC_MESSAGES
	msgfmt messages.po
	cd ../../..
done

[ ! -d xhtml ] && mkdir xhtml
rm -rf xhtml/*
cp style.css xhtml
cp index.php xhtml
mkdir xhtml/images
cp $DATADIR/pixmaps/florence.svg xhtml/images
setxkbmap -print >/tmp/xkbmap.tmp
doc C english C us
doc ru russian ru_RU.UTF8 ru
doc fr francais fr_FR.UTF8 fr
cat /tmp/xkbmap.tmp | xkbcomp - $DISPLAY 2>/dev/null && rm /tmp/xkbmap.tmp

