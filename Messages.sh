#! /bin/sh
$PREPARETIPS > tips.cpp
$XGETTEXT *.cpp -o $podir/kdvi.pot
rm -f tips.cpp
