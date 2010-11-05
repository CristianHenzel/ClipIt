#!/bin/sh

echo "gettextize..."

GETTEXTIZE="gettextize"
($GETTEXTIZE --version) < /dev/null > /dev/null 2>&1 || {
	echo "gettextize not found"
	exit 1
}

if test "$GETTEXTIZE"; then
	echo "Creating $dr/aclocal.m4 ..."
	test -r aclocal.m4 || touch aclocal.m4
	echo "Running $GETTEXTIZE...  Ignore non-fatal messages."
	echo "no" | $GETTEXTIZE --force --copy
	echo "Making aclocal.m4 writable ..."
	test -r aclocal.m4 && chmod u+w aclocal.m4
fi

echo "intltoolize..."
(intltoolize --version) < /dev/null > /dev/null 2>&1 || {
	echo
	echo "intltoolize not found"
	exit 1
}

intltoolize --copy --force --automake

echo "aclocal..."
(aclocal --version) < /dev/null > /dev/null 2>&1 || {
	echo "aclocal not found"
	exit 1
}

aclocal -I m4

echo "autoheader..."
(autoheader --version) < /dev/null > /dev/null 2>&1 || {
	echo "autoheader not found"
	exit 1
}

autoheader

echo "automake..."
(automake --version) < /dev/null > /dev/null 2>&1 || {
	echo "automake not found"
	exit 1
}

automake --add-missing --copy --gnu

echo "autoconf..."
(autoconf --version) < /dev/null > /dev/null 2>&1 || {
	echo "autoconf not found"
	exit 1
}

autoconf

echo "now run configure"

exit 0
