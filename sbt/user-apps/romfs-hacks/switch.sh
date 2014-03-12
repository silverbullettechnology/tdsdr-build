#!/bin/sh

# some basic setup
mkdir -p /dev/mapper /proc
mount -tproc     proc /proc
mount -tdevtmpfs udev /dev

# if no root= specified on the command-line, drop to shell here 
if ! grep -q 'root=' /proc/cmdline; then
	echo "No root= in kernel command-line, stop"
	exec /bin/sh
fi

# Extract root= from kernel cmdline and setup opts
root=`cat /proc/cmdline | sed -e 's/^.*root=//' -e 's/ .*$//'`
opts=noatime,nodiratime

# Workaround for bug in BB grep -w (fails when line starts with "root=")
cat /proc/cmdline | tr -s ' ' '\n' | grep -q '^ro$' && opts="$opts,ro"

# try mount with retry
mkdir -p /new-root
for try in 1 2 3 4 5; do
	mount $root /new-root -o$opts
	grep -q /new-root /proc/mounts && break;
	sleep 1
done

# not mounted - drop to a shell
if ! grep -q /new-root /proc/mounts; then
	echo "$root: not found"
	exec /bin/sh
fi

# it worked, do a switch-root to it
echo "Switch to new root..."
mount --move /proc /new-root/proc
mount --move /dev  /new-root/dev
exec switch_root /new-root /sbin/init

