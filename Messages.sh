#!/bin/sh
$EXTRACTRC *.rc */*.rc >> rc.cpp || exit 11
$EXTRACTRC $(find conf/ -name "*.ui") $(find core/ -name "*.ui") $(find ui/ -name "*.ui") $(ls . | grep -E '\.ui') >> rc.cpp || exit 12
$XGETTEXT $(find conf/ -name "*.cpp" -o -name "*.h") $(find core/ -name "*.cpp" -o -name "*.h") $(find ui/ -name "*.cpp" -o -name "*.h") $(find shell/ -name "*.cpp" -o -name "*.h") $(ls . | grep -E '\.cpp$') $(ls . | grep -E '\.h$') -o $podir/okular.pot
