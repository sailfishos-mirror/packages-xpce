#!/bin/sh -f
#
# Ask PceEmacs to edit files for  me.   To  use  this package, make sure
# xpce-client is installed in  your  path   and  one  XPCE  process runs
# PceEmacs as a server. One way to do   that  is to put the following in
# your X startup:
#
#      xterm -iconic -title 'PceEmacs server' -e swipl -g emacs_server
#
# Author: Jan Wielemaker
# Copying: Public domain

usage()
{ echo "usage: `basename $0` [+line] file[:line]"
  exit 1
}

line=""

HOST=`hostname`
APPCONFIG=${XDG_DATA_HOME-$HOME/.config}/swi-prolog/xpce
serverbase="$APPCONFIG/emacs_server"

case "$DISPLAY" in
  :*)	server=$serverbase.$(echo "$DISPLAY" | sed 's/^:\([0-9]*\).*/\1/')
	;;
  *)	server=$serverbase.$HOST
        ;;
esac

#echo "Server = $server"

if [ ! -S "$server" -o -z "$DISPLAY" ]; then
    server="$serverbase"
    if [ ! -S "$server" ]; then
	echo "No PceEmacs server"
	exec emacs "$*"
    fi
fi

done=false
while [ $done = false ]; do
    case "$1" in
	+[0-9]*)
		line=`echo "$1" | sed 's/^+//'`
		shift
		;;
	*)
		done=true
		;;
    esac
done

file="$1"

case "$file" in
	"")	usage
		;;
	[~/]*)	;;
	*)	file=`pwd`/$file ;;
esac

case "$file" in
	*:[0-9]*:[0-9]*)
		eval `echo $file | sed 's/\(.*\):\([0-9]*\):\([0-9]*\)$/file=\1;line=\2;charpos=\3/'`
		;;
	*:[0-9]*)
		eval `echo $file | sed 's/\(.*\):\([0-9]*\)$/file=\1;line=\2/'`
		;;
esac

if [ -z "$line" ]; then
	cmd="edit('$file')"
elif [ -z "$charpos" ]; then
	cmd="edit('$file', $line)"
else
	cmd="edit('$file', $line, $charpos)"
fi

xpce-client $server -bc "$cmd"

if [ $? = 2 ]; then
  if [ "$line" = "" ]; then
    exec emacs "$file";
  else
    exec emacs "$file" +$line
  fi
fi
