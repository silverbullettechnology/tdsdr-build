#!/bin/sh

in_table="gpio-setup.$1"
out_table="$ROMFSDIR/etc/gpio-setup"

rm -f "$ROMFSDIR/etc/rcS.d/S30gpio-setup"
if [ -s "$in_table" ]; then
	install -v -m644 "$in_table" "$out_table"
	install -v -m755 gpio-setup.sh  "$ROMFSDIR/etc/init.d/gpio-setup"
	ln -sv ../init.d/gpio-setup  "$ROMFSDIR/etc/rcS.d/S30gpio-setup"
else
	echo "No GPIO table for this board"
fi
