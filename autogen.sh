#!/bin/sh
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
	echo "You need the $1 command to execute $COMMAND. Please install $1 and rerun $COMMAND"
	exit 1
}

# check for the presence of a command
function check {
	which $1 >/dev/null 2>&1
	if [ $? -ne 0 ]
	then
		fatal $1
	fi
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

# check for which
which which >/dev/null 2>&1
if [ $? -ne 0 ]
then
	fatal which
fi

run aclocal
CURR_PWD=$PWD
run glib-gettextize --force --copy
run intltoolize --copy --force --automake
run autoheader
run automake
run autoconf
cd data
run trang -I rnc -O rng florence.rnc relaxng/florence.rng

# go back
cd $OLD_PWD

