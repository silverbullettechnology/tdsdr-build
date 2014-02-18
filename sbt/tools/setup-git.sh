#!/bin/sh
# SBT Kernel Git setup script for SBT SDRDC 
# Copyright (C) 2014 Silver Bullet Technology

[ -z "$SBT_TOP" ] && exit 1

cwd=`pwd`
mkdir -p "$SBT_KERNEL"
cd       "$SBT_KERNEL"

md5=`echo -n "$SBT_KERNEL_GIT_REPO" | md5sum | cut -c1-32`
dir="$SBT_KERNEL/$md5"
if ! [ -d "$dir" ]; then
	if ! git clone "$SBT_KERNEL_GIT_REPO" "$md5"; then
		echo "Git clone of $SBT_KERNEL_GIT_REPO failed, stopping here."
		exit 1
	fi
fi
cd "$dir"

if [ -n "$SBT_KERNEL_GIT_USER" ]; then
	old=`git config user.name`
	if [ "$old" != "$SBT_KERNEL_GIT_USER" ]; then
		if git config user.name "$SBT_KERNEL_GIT_USER"; then
			echo "user.name changed: '$old' -> '$SBT_KERNEL_GIT_USER'"
		fi
	fi
fi

if [ -n "$SBT_KERNEL_GIT_EMAIL" ]; then
	old=`git config user.email`
	if [ "$old" != "$SBT_KERNEL_GIT_EMAIL" ]; then
		if git config user.email "$SBT_KERNEL_GIT_EMAIL"; then
			echo "user.email changed: '$old' -> '$SBT_KERNEL_GIT_EMAIL'"
		fi
	fi
fi

if [ -n "$SBT_KERNEL_GIT_BRANCH" ]; then
	loc=`git branch -a | grep -c "^. $SBT_KERNEL_GIT_BRANCH\$"`
	rem=`git branch -a | grep -c "origin/$SBT_KERNEL_GIT_BRANCH\$"`

	if [ "$loc" = "0" ] && [ "$rem" = "0" ]; then
		echo "Creating remote tracking branch $SBT_KERNEL_GIT_BRANCH"
		if ! git branch --track "$SBT_KERNEL_GIT_BRANCH" "origin/$SBT_KERNEL_GIT_BRANCH"; then
			echo "Creating tracking branch failed, stopping here."
			exit 1
		fi
		if ! git checkout "$SBT_KERNEL_GIT_BRANCH"; then
			echo "Checkout of tracking branch failed, stopping here."
			exit 1
		fi
	elif [ "$loc" = "1" ]; then
		echo "Local branch $SBT_KERNEL_GIT_BRANCH was already setup, no action taken."
	fi
		
	cur=`git branch -a | grep -c "^\* $SBT_KERNEL_GIT_BRANCH\$"`
	if [ "$cur" = "0" ]; then
		git checkout "$SBT_KERNEL_GIT_BRANCH"
	fi
fi

cd "$SBT_KERNEL"
rm -f current
ln -s $md5 current

cd "$cwd"
exit 0
