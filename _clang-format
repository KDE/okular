---
# SPDX-License-Identifier: MIT
#
# Copyright (C) 2019 Christoph Cullmann <cullmann@kde.org>
# Copyright (C) 2019 Gernot Gebhard <gebhard@absint.com>
#
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
# CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

# Style for C++
Language: Cpp

# base is WebKit coding style: https://webkit.org/code-style-guidelines/
# below are only things set that diverge from this style!
BasedOnStyle: WebKit

# enforce C++11 (e.g. for std::vector<std::vector<lala>>
Standard: Cpp11

# 4 spaces indent
TabWidth: 4

# 3 * 80 wide lines
ColumnLimit: 240

# sort includes inside line separated groups
SortIncludes: true

# break before braces on function, namespace and class definitions.
BreakBeforeBraces: Linux

# CrlInstruction *a;
PointerAlignment: Right

# horizontally aligns arguments after an open bracket.
AlignAfterOpenBracket: Align

# align trailing comments
AlignTrailingComments: true

# don't move all parameters to new line
AllowAllParametersOfDeclarationOnNextLine: false

# no single line functions
AllowShortFunctionsOnASingleLine: None

# always break before you encounter multi line strings
AlwaysBreakBeforeMultilineStrings: true

# don't move arguments to own lines if they are not all on the same
BinPackArguments: false

# don't move parameters to own lines if they are not all on the same
BinPackParameters: false

# don't break binary ops
BreakBeforeBinaryOperators: None

# format C++11 braced lists like function calls
Cpp11BracedListStyle: true

# remove empty lines
KeepEmptyLinesAtTheStartOfBlocks: false

# no namespace indentation to keep indent level low
NamespaceIndentation: None

# we use template< without space.
SpaceAfterTemplateKeyword: false

# macros for which the opening brace stays attached.
ForEachMacros: [ foreach, Q_FOREACH, BOOST_FOREACH, forever, Q_FOREVER, QBENCHMARK, QBENCHMARK_ONCE ]
