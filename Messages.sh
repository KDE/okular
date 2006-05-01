#! /bin/sh
$EXTRACTRC `find . -name "*.rc" -o -name "*.ui"` >> rc.cpp || exit 11
$XGETTEXT `find . -name "*.cpp" -o -name "*.cc" -o -name "*.h"` -o $podir/kpdf.pot
