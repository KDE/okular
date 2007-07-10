#!/bin/sh
$EXTRACTRC $(find conf/ -name "*.ui") >> rc_okular_ghostview.cpp || exit 11
$XGETTEXT $(find . -name "*.cpp" -o -name "*.h") -o $podir/okular_ghostview.pot
