#!/bin/bash
# 
#  Florence - Florence is a simple virtual keyboard for Gnome.
#
#  Copyright (C) 2008 François Agrech
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2, or (at your option)
#  any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software Foundation,
#  Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  
#

export COMMAND="$0"

# exit on missing command
function fatal {
	echo "You need the $1 command to execute $COMMAND. Please install $1 and rerun $COMMAND" >&2
	exit 1
}

# check for the presence of a command
function check {
	hash $1 2>&- >/dev/null || fatal $1
}

# execute action and print a message
function run {
	check $1
	echo "executing \"$*\""
	eval $* >/dev/null
}

# go to project directory
OLD_PWD=$PWD
cd ${0%%/*}

run aclocal
CURR_PWD=$PWD
run gnome-doc-prepare --force
run glib-gettextize --force --copy
run intltoolize --copy --force --automake
run autoheader
run automake --foreign
run autoconf
cd data
hash trang 2>&- >/dev/null
if [ $? -eq 0 ]; then
	run trang -I rnc -O rng florence.rnc relaxng/florence.rng
	run trang -I rnc -O rng style.rnc relaxng/style.rng
else
	test "x$TRANGPATH" = "x" && echo "Please set the TRANGPATH environment variable." >&2 && exit 1
	[ ! -e $TRANGPATH/trang.jar ] &&
		echo "You need trang to execute $COMMAND. please install trand and rerun $COMMAND." &&
		exit 1
	run java -jar $TRANGPATH/trang.jar -I rnc -O rng florence.rnc relaxng/florence.rng
	run java -jar $TRANGPATH/trang.jar -I rnc -O rng style.rnc relaxng/style.rng
fi

# go back
cd $OLD_PWD

