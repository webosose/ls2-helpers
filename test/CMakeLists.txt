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

include_directories(${CMAKE_SOURCE_DIR}/include/public)

set(MEMORYCHECK_COMMAND_OPTIONS "--tool=memcheck --leak-check=full --trace-children=yes")
set(MEMORYCHECK_SUPPRESSIONS_FILE "${CMAKE_SOURCE_DIR}/valgrind.supp")

webos_use_gtest()

set(UNIT_TEST_SOURCES
    test_jsonparser
    )

set(INTEGRATION_TEST_SOURCES
    test_c_api
    test_servicepoint_service
    test_servicepoint_client
    test_servicepoint_signal
    test_subscriptionpoint
    test_persistentsubscription
    )

set(TEST_LIBRARIES
        ${PROJECT_NAME}
        ${TESTLIBNAME}
        ${WEBOS_GTEST_LIBRARIES}
        ${LS2_LIBRARIES}
        ${PBNJSON_CPP_LIBRARIES}
        )

foreach(TEST ${UNIT_TEST_SOURCES})
    add_executable(${TEST} ${TEST}.cpp)
    target_link_libraries(${TEST} ${PROJECT_NAME} ${TEST_LIBRARIES})
    add_test(${TEST} ${TEST})
endforeach()


add_integration_test_cases("integration" "${INTEGRATION_TEST_SOURCES}" "${TEST_LIBRARIES}")
