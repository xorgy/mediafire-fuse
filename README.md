About
=====

The mediafire-tools project offers two programs to interact with a mediafire
account:

 - mediafire-shell: a simple shell for a mediafire account like ftp(1)
 - mediafire-fuse: a fuse module that is able to mount the mediafire share locally

License
=======

This software is copyright 2013-2014 Bryan Christ and Johannes Schauer and can
re redistributed and/or modified under the terms of the GNU General Public
License version 2.

For the exact terms of the license, see `COPYING`.

Compiling
=========

On Debian and derivatives like Ubuntu you need the following packages to build
this software:

	apt-get install cmake build-essential libjansson-dev libcurl4-openssl-dev libfuse-dev libssl-dev

On FreeBSD you need:

	pkg install jansson cmake curl fuse fusefs-libs

This project is using cmake, so you can build the software out-of-tree:

	mkdir build
	cd build
	cmake ..
	make
	make install

Runtime Configuration
=====================

Both tools are configured through a configuration file at
`$XDG_CONFIG_HOME/mediafire-tools/config`. In case the variable
`$XDG_CONFIG_HOME` is not set, the configuration file lives at
`~/.config/mediafire-tools/config`. The format of the configuration file is the
same as the arguments given on the command line except that you can use
newlines and lines starting with a # (hash) are ignored. Thus the following is
a valid configuration file:

	# this line is ignored
	--username foo@bar.com
	--password secret

And so is this:

	--username foo@bar.com -p secret

Arguments given on the commandline will overwrite arguments given in the
configuration file.

You should not specify your password on the commandline as it will then be
visible by other processes. If the password is neither specified on the command
line nor in the configuration file, then it will be prompted via standard
input.

Mediafire Shell
===============

This program serves as a simple ftp(1) like interface to a mediafire share. It
could thus also be used as part of scripts. For a list of available commandline
options, run it with the `--help` argument. For a list of available commands,
run the `help` command either in interactive mode or by giving it to the `-c`
argument.

Mediafire Fuse
==============

The fuse module allows to mount a mediafire share at a local mountpoint.

It maintains a local cache of the directory structure as well as the file
content. The directory structure cache can be found in
`~/.cache/mediafire-tools/<ekey>/directorytree` where `<ekey>` is the unique id
of your username. The file cache can be found in
`~/.cache/mediafire-tools/<ekey>/files/`.

You can mount the module like this:

	./mediafire-fuse /mnt

And unmount it like this:

	fusermount -u /mnt

Bugs
====

If you encounter a bug, try to reproduce it while running `mediafire-fuse` with
the `-d` option and send in the generated output on the terminal.

While the fuse filesystem is not mounted, it is always safe to remove your
local cache as it will be retrieved again from the remote.

TODO
====

For missing features that could be implemented in the future, have a look at
the file `TODO`.
