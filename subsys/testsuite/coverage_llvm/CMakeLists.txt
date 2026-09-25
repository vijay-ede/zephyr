# SPDX-License-Identifier: Apache-2.0
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.

# LLVM source-based coverage runtime for bare-metal / QEMU targets.
#
# This module:
#   1. Compiles coverage_llvm.c (the serial dump runtime).
#   2. Links the bare-metal libclang_rt.profile.a (provides
#      __llvm_profile_get_size_for_buffer and __llvm_profile_write_buffer).
#   3. Injects linker section fragments so that:
#      - __llvm_prf_cnts and __llvm_prf_data land in writable RAM
#        (RAM_SECTIONS → snippets-ram-sections.ld, inside RAMABLE_REGION)
#      - __llvm_prf_names, __llvm_covmap, __llvm_covfun land in read-only
#        ROM (RODATA → snippets-rodata.ld, inside ROMABLE_REGION)
#
# The RAM placement is critical: without it, the RISC-V PMP marks the
# text/rodata region as read+execute only, and the very first counter
# store during early boot (before PMP is configured) causes a
# Store/AMO access fault.

zephyr_sources(coverage_llvm.c)

# Force the __llvm_profile_* symbols into the link so that the linker
# does not discard the profile sections as unreferenced.
zephyr_link_libraries(
  -Wl,-u,__llvm_profile_get_size_for_buffer
  -Wl,-u,__llvm_profile_write_buffer
)

# Locate the bare-metal libclang_rt.profile.a.
#
# Strategy: ask the compiler for the path to libclang_rt.builtins.a
# (which it always knows), then substitute "builtins" with "profile".
# This correctly handles all multilib variants (rv32imac, rv32imafc, etc.)
# without hard-coding the path.
#
# We pass the target triple and arch/ABI flags so the compiler selects
# the correct multilib directory.
set(_clang_rt_extra_flags "")
if(CMAKE_C_COMPILER_TARGET)
  list(APPEND _clang_rt_extra_flags "--target=${CMAKE_C_COMPILER_TARGET}")
endif()
if(DEFINED riscv_march)
  list(APPEND _clang_rt_extra_flags "-march=${riscv_march}")
endif()
if(DEFINED riscv_mabi)
  list(APPEND _clang_rt_extra_flags "-mabi=${riscv_mabi}")
endif()

execute_process(
  COMMAND ${CMAKE_C_COMPILER}
    ${_clang_rt_extra_flags}
    --rtlib=compiler-rt
    -print-libgcc-file-name
  OUTPUT_VARIABLE _clang_rt_builtins_path
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_QUIET
)

if(_clang_rt_builtins_path)
  string(REPLACE "libclang_rt.builtins" "libclang_rt.profile"
         _clang_rt_profile_path "${_clang_rt_builtins_path}")

  if(EXISTS "${_clang_rt_profile_path}")
    zephyr_link_libraries("${_clang_rt_profile_path}")
    message(STATUS "LLVM coverage: using ${_clang_rt_profile_path}")
  else()
    message(WARNING
      "LLVM coverage: libclang_rt.profile.a not found at "
      "${_clang_rt_profile_path}. "
      "Set EXTRA_LDFLAGS to add it manually.")
  endif()
else()
  message(WARNING
    "LLVM coverage: could not determine libclang_rt.builtins.a path. "
    "Set EXTRA_LDFLAGS to add libclang_rt.profile.a manually.")
endif()

# Inject linker section fragments.
#
# DATA_SECTIONS: __llvm_prf_cnts, __llvm_prf_bitmap, __llvm_prf_data
#   → placed after initialized data sections to ensure they're in the second
#     LOAD segment (RAM segment), not the first LOAD segment (ROM segment).
#     This is critical for non-XIP systems where ROMABLE_REGION == RAMABLE_REGION.
#     Without this, __llvm_prf_cnts ends up in the first LOAD segment which is
#     treated as read-only by the linker, causing Store/AMO access faults.
#
# ROM_SECTIONS: __llvm_prf_names
#   → dedicated output section in ROMABLE_REGION (generates boundary symbols)
#
# SECTIONS: __llvm_covmap, __llvm_covfun
#   → non-allocatable, address 0, outside all memory GROUPs
#   → retained in ELF for llvm-cov host tool (same pattern as .debug_info)
#
# When CONFIG_USERSPACE is enabled, coverage_llvm.c registers a
# k_mem_partition covering __llvm_prf_cnts and __llvm_prf_bitmap and adds it
# to the default memory domain via SYS_INIT(PRE_KERNEL_1). This grants
# user-mode threads write access to the coverage counters without requiring
# any linker script changes.
if(CONFIG_DEMAND_PAGING)
  zephyr_linker_sources(PINNED_DATA_SECTIONS coverage_llvm_ram.ld)
  zephyr_linker_sources(PINNED_RODATA coverage_llvm_rodata.ld)
else()
  zephyr_linker_sources(DATA_SECTIONS coverage_llvm_ram.ld)
  zephyr_linker_sources(DATA_SECTIONS coverage_llvm_data.ld)
  zephyr_linker_sources(ROM_SECTIONS coverage_llvm_rodata.ld)
  zephyr_linker_sources(SECTIONS coverage_llvm_sections.ld)
endif()
