#!/bin/sh
$EXTRACTRC *.rc */*.rc >> rc.cpp || exit 11
$EXTRACTRC $(find . -name "*.ui") >> rc.cpp || exit 12
$XGETTEXT $(find . -name "*.cpp" -o -name "*.h") -o $podir/okular.pot
