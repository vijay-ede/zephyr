# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2024 Qualcomm Innovation Center, Inc.

add_subdirectory_ifdef(CONFIG_ZTEST ztest)

if(CONFIG_TEST)
  zephyr_include_directories(${ZEPHYR_BASE}/subsys/testsuite/include)
endif()
add_subdirectory_ifdef(CONFIG_COVERAGE_GCOV coverage)
zephyr_include_directories_ifdef(CONFIG_COVERAGE_GCOV ${ZEPHYR_BASE}/subsys/testsuite/coverage)
add_subdirectory_ifdef(CONFIG_COVERAGE_LLVM_SOURCE coverage_llvm)

zephyr_library_sources_ifdef(CONFIG_TEST_BUSY_SIM busy_sim/busy_sim.c)
zephyr_library_sources_ifdef(CONFIG_TEST_CAPTURE capture/tc_capture.c)
