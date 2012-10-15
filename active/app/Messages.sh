#! /usr/bin/env bash
$EXTRACTRC `find . -name \*.rc -o -name \*.ui -o -name \*.kcfg` >> rc.cpp
$XGETTEXT `find . -name \*.qml` -L Java -o $podir/org.kde.active.documentviewer.pot
$XGETTEXT `find src/ -name "*.cpp" -o -name "*.h"` -o $podir/org.kde.active.documentviewer.pot
rm -f rc.cpp
