#!/bin/sh
#
# This scripts adds local version information from the version
# control systems git, mercurial (hg) and subversion (svn).
#
# If something goes wrong, send a mail the kernel build mailinglist
# (see MAINTAINERS) and CC Nico Schottelius
# <nico-linuxsetlocalversion -at- schottelius.org>.
#
#

srctree=.

git_version()
{
	cd "$srctree"

	# Check for git and a git repo.
	if test -n "$(git rev-parse --show-cdup 2>/dev/null)" &&
	   head=`git rev-parse --verify --short HEAD 2>/dev/null`; then

		if [ -z "`git describe --exact-match 2>/dev/null`" ]; then
			branch=`git symbolic-ref --short HEAD`
			date="$(git log -1 --format=%ci | sed 's/\s.*$//' | sed 's/-//g' )"

			printf '%s-%s-g%s' $branch $date $head
		fi

		# Check for uncommitted changes
		if git diff-index --name-only HEAD | grep -qv "^scripts/package"; then
			printf '%s' -dirty
		fi

		# All done with git
		return
	fi
}

if test $# -gt 0; then
	srctree=$1
fi

res="$(git_version)"
echo "$res" >.gitversion

echo "$res"
