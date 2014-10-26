#!/bin/sh

set -ex

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

cmd="valgrind --suppressions="${source_dir}/valgrind.supp" --fullpath-after="$source_dir" --tool=memcheck --leak-check=yes --show-reachable=yes --num-callers=20 --track-fds=yes --track-origins=yes --error-exitcode=1 --quiet"

if [ ! -f "./.mediafire-tools.conf" -a ! -f "~/.mediafire-tools.conf" ]; then
	echo "no configuration file found" >&2
	exit 1
fi

if [ `mount -t fuse.mediafire-fuse | wc -l` -ne 0 ]; then
	echo "a fuse fs is already mounted" >&2
	exit 1
fi

$cmd "${binary_dir}/mediafire-fuse" -s -f -d /mnt &
fusepid="$!"

# once the file system is found to be mounted, print the tree and unmount
for i in `seq 1 10`; do
	sleep 1
	if [ `mount -t fuse.mediafire-fuse | wc -l` -ne 0 ]; then
		break;
	fi
done

if [ `mount -t fuse.mediafire-fuse | wc -l` -eq 0 ]; then
	echo "cannot mount fuse" >&2
	fusermount -u /mnt
	wait "$fusepid"
	exit 1
fi

tree /mnt

fusermount -u /mnt

wait "$fusepid"
