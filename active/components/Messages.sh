#! /usr/bin/env bash
$XGETTEXT `find src/ -name "*.cpp" -o -name "*.h"` -o $podir/org.kde.okular.pot
rm -f rc.cpp
