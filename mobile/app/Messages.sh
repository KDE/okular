#! /usr/bin/env bash
$EXTRACTRC `find . -name \*.rc -o -name \*.ui -o -name \*.kcfg` >> rc.cpp
$XGETTEXT `find . -name \*.qml` -L Java -o $podir/org.kde.active.documentviewer.temp.pot
$XGETTEXT `find src/ -name "*.cpp" -o -name "*.h"` -o $podir/org.kde.active.documentviewer.pot
$MSGCAT $podir/org.kde.active.documentviewer.temp.pot $podir/org.kde.active.documentviewer.pot -o $podir/org.kde.active.documentviewer.pot
rm -f $podir/org.kde.active.documentviewer.temp.pot
rm -f rc.cpp
