#!/bin/sh

ret=0
find .. -name "*.c" -o -name "*.h" | while read f; do
	case $f in
		../*/CMakeFiles/*)
			;;
		*)
			INDENT_PROFILE=../.indent.pro indent -st $f | diff -u $f -
			ret=$((ret+$?))
			;;
	esac
done

exit $ret
