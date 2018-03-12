ls2-helpers
=============

Summary
-------
Open webOS Luna System Bus helper library for C++11.

Description
-----------

A helper library to make development of native LS2 services easier and more concise.
Implements an wrapper around LS2 C API.

How to Build on Linux
=====================

## Dependencies

Below are the tools and libraries (and their minimum versions) required to build
_ls2-helpers_:

* cmake (version required by webosose/cmake-modules-webos)
* gcc 4.7.0
* glib-2.0 2.32.1
* make (any version)
* webosose/cmake-modules-webos 1.2.0
* webosose/libpbnjson_cpp 2.11.0
* webosose/PmLogLib 3.0.2
* webosose/luna-service2 3.15.0
* pkg-config 0.26

Below are the tools (and their minimum versions) required to test _ls2-helpers_:

* gtest 1.7.0

## Building

Once you have downloaded the source, enter the following to build it (after
changing into the directory under which it was downloaded):

    $ mkdir BUILD
    $ cd BUILD
    $ cmake ..
    $ make
    $ sudo make install

The directory under which the files are installed defaults to `/usr/local/webos`.
You can install them elsewhere by supplying a value for `WEBOS_INSTALL_ROOT`
when invoking `cmake`. For example:

    $ cmake -D WEBOS_INSTALL_ROOT:PATH=$HOME/projects/webosose ..
    $ make
    $ make install

will install the files in subdirectories of `$HOME/projects/webosose`.

Specifying `WEBOS_INSTALL_ROOT` also causes `pkg-config` to look in that tree
first before searching the standard locations. You can specify additional
directories to be searched prior to this one by setting the `PKG_CONFIG_PATH`
environment variable.

If not specified, `WEBOS_INSTALL_ROOT` defaults to `/usr/local/webos`.

To configure for a debug build treating warnings as errors, enter:

    $ cmake -D CMAKE_BUILD_TYPE:STRING=Debug -D WEBOS_USE_WERROR:BOOL=TRUE ..

To see a list of the make targets that `cmake` has generated, enter:

    $ make help

## Uninstalling

From the directory where you originally ran `make install`, enter:

    $ [sudo] make uninstall

You will need to use `sudo` if you did not specify `WEBOS_INSTALL_ROOT`.

## Generating Documentation

The tools required to generate the documentation are:

- doxygen 1.7.6.1
- graphviz 2.26.3

To generate the documentation, add `-D WEBOS_CONFIG_BUILD_DOCS:BOOL=TRUE` to the `cmake`
command line and make the `docs` target:

    $ cmake -D WEBOS_CONFIG_BUILD_DOCS:BOOL=TRUE <other-args> ..
    $ make docs

To view the generated HTML documentation, point your browser to
`Documentation/ls2/doc/*/index.html`

## Testing

To enable tests add `-D WEBOS_CONFIG_BUILD_TESTS:BOOL=TRUE` to the `cmake`
command line. Add `-D WEBOS_GTEST_SRCDIR:STRING=/path/to/gtest` to specify path
to gtest source directory, default is `WEBOS_INSTALL_ROOT/src/gtest`.

    $ cmake -DWEBOS_CONFIG_BUILD_TESTS=TRUE \
      -DWEBOS_GTEST_SRCDIR=/path/to/gtest <other-args> ..
    $ make test

# Copyright and License Information

Unless otherwise specified, all content, including all source code files and
documentation files in this repository are:

Copyright (c) 2016-2018 LG Electronics, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

SPDX-License-Identifier: Apache-2.0
