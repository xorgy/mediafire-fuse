#!/bin/sh

ret=0
for f in ../**/*.c ../**/*.h; do
	case $f in
		../**/CMakeFiles/**/*)
			;;
		*)
			INDENT_PROFILE=../.indent.pro indent -st $f | diff -u $f -
			ret=$((ret+$?))
			;;
	esac
done

exit $ret
