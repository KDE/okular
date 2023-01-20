# Okular – Universal Document Viewer

Okular can view and annotate documents of various formats, including PDF, Postscript, Comic Book, and various image formats.
It supports native PDF annotations.

## Download

For download and installation instructions, visit this [link](https://okular.kde.org/download.php).

## User Manual

The user manual for Okular can be found [here](https://docs.kde.org/?application=okular&branch=stable5).

## Bugs

To view the buglist for Okular, visit this [link](https://bugs.kde.org/buglist.cgi?product=okular).

**Please report bugs on [Bugzilla](https://bugs.kde.org/enter_bug.cgi?product=okular), and not on our [GitLab instance](https://invent.kde.org).**

## Mailing List

To subscribe to Okular-devel, visit this [link](https://mail.kde.org/mailman/listinfo/okular-devel).

## Source Code

The source code for Okular can be found at this [link](https://invent.kde.org/graphics/okular.git).

The Okular repository contains source code for:
 * The `okular` desktop application (the “shell”),
 * The `okularpart` KParts plugin,
 * The `okularkirigami` mobile application,
 * Several `okularGenerator_xyz` plugins, which provide backends for different document types.

## API Documentation

 You can visit this [link](https://api.kde.org/okular/html/index.html) for API documentation.

## Contributing

Okular uses the merge request workflow.
Merge requests are required to run pre-commit CI jobs; **please don’t push to the master branch directly.**
See [Infrastructure/GitLab](https://community.kde.org/Infrastructure/GitLab) for an introduction.

## Build Instructions

Okular can be built like many other applications developed by KDE.
See [Get_Involved/development](https://community.kde.org/Get_Involved/development) for an introduction.

If your build environment is set up correctly, you can also build Okular using CMake:

```bash
git clone https://invent.kde.org/graphics/okular.git
cd okular
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=/path/to/your/install/dir ..
make
make install
```

Okular also builds tests in the build tree. To run them, you have to run `make install` first.

If you install Okular in a different path than your system install directory, it is possible that you need to run

```bash
source prefix.sh
```

so that the correct Okular instance and libraries are picked up.
Afterwards one can run `Okular` inside the shell instance.
The source command is also required to run the tests manually.

As stated above, Okular has various build targets and two of them are executables.
You can choose which executable to build by passing a flag to CMake:

```bash
cmake -DCMAKE_INSTALL_PREFIX=/path/to/your/install/dir -DOKULAR_UI=desktop ..
```
Available options for the flag are `desktop`, `mobile`, and `both`.

## clang-Format

The Okular project uses clang-format to enforce source code formatting.
See [README.clang_format](https://invent.kde.org/graphics/okular/-/blob/master/README.clang-format) for more information.
