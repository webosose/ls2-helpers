# Copyright (c) 2016-2018 LG Electronics, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 2.8.7)
project(ls2-helpers)

include(webOS/webOS)

webos_modules_init(1 3 0)
webos_component(1 0 0)

set(WEBOS_USE_WERROR FALSE CACHE BOOL "Set to TRUE to enable -Werror")
if(WEBOS_USE_WERROR)
    message("Build with treating most warnings as errors")
    webos_add_compiler_flags(ALL
            -Werror -Wextra
            -Wno-error=missing-field-initializers
            -Wno-error=unused-parameter
            -Wno-error=sign-compare
            -Wno-error=type-limits
            )
endif()

# Turn on all warnings
webos_add_compiler_flags(ALL -std=c++11 -Wall -Wextra -fno-permissive)
#disable specific warnings
webos_add_compiler_flags(ALL -Wno-unused-parameter -Wno-type-limits, -Wno-missing-field-initializers)
#promote specific warnings to errors
webos_add_compiler_flags(ALL -Werror=return-type  -Werror=reorder -Werror=uninitialized)
# Require that all undefined symbols are satisfied by the libraries from target_link_libraries()
webos_add_linker_options(ALL --no-undefined)


include(FindPkgConfig)

include_directories(src)
include_directories(include/public/ls2-helpers)

# -- check for glib 2.0
pkg_check_modules(GLIB2 REQUIRED glib-2.0)
include_directories(${GLIB2_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${GLIB2_CFLAGS})

# check for LS2
pkg_check_modules(LS2 REQUIRED luna-service2++)
include_directories(${LS2_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${LS2_CFLAGS_OTHER})

# check for pbnjson cpp library
pkg_check_modules(PBNJSON_CPP REQUIRED pbnjson_cpp)
include_directories(${PBNJSON_CPP_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${PBNJSON_CPP_CFLAGS})

# check for pmloglib library
pkg_check_modules(PMLOG REQUIRED PmLogLib>=3)
include_directories(${PMLOG_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${PMLOG_CFLAGS})

link_directories(${GLIB2_LIBRARY_DIRS}
                 ${LS2_LIBRARY_DIRS}
                 ${PBNJSON_CPP_LIBRARY_DIRS}
                 ${PMLOG_LIBRARY_DIRS})


set(WEBOS_CONFIG_BUILD_TESTS FALSE CACHE BOOL "Set to TRUE to enable tests compilation")
if (WEBOS_CONFIG_BUILD_TESTS)
    include(CTest)
    add_subdirectory(files/test)
    add_subdirectory(test)
    add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -E 'integration|test_clock|test_timersource')
else()
    message(STATUS "libls2-helpers: skipping automatic tests")
endif()

if(WEBOS_CONFIG_BUILD_DOCS)
	add_subdirectory(doc)
else()
	message(STATUS "Skipping document generation")
endif()

add_subdirectory(src)

webos_build_pkgconfig(files/pkgconfig/${CMAKE_PROJECT_NAME})
