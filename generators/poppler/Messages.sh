#!/bin/sh
$EXTRACTRC $(find conf/ -name "*.ui" -o -name "*.kcfg") >> rc_okular_poppler.cpp || exit 11
$XGETTEXT $(find . -name "*.cpp" -o -name "*.h") -o $podir/okular_poppler.pot
rm -f rc_okular_poppler.cpp
