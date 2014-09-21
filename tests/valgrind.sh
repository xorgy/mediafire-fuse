#!/bin/sh

case $# in
	0)
		source_dir="."
		binary_dir="."
		;;
	2)
		source_dir=$1
		binary_dir=$2
		;;
	*)
		echo "usage: $0 [source_dir] [binary_dir]"
		exit 1
		;;
esac

cmd="valgrind --suppressions="${source_dir}/valgrind.supp" --fullpath-after="$source_dir" --tool=memcheck --leak-check=yes --show-reachable=yes --num-callers=20 --track-fds=yes --track-origins=yes"

$cmd "${binary_dir}/mediafire-shell" -c "help; whoami"
