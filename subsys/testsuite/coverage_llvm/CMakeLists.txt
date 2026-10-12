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
zephyr_linker_sources(DATA_SECTIONS coverage_llvm_ram.ld)
zephyr_linker_sources(DATA_SECTIONS coverage_llvm_data.ld)
zephyr_linker_sources(ROM_SECTIONS coverage_llvm_rodata.ld)
zephyr_linker_sources(SECTIONS coverage_llvm_sections.ld)
