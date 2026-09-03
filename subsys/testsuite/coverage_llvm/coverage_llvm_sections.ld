/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Preserve LLVM coverage mapping sections for llvm-cov host tool.
 *
 * __llvm_covmap and __llvm_covfun are non-allocatable sections (no SHF_ALLOC).
 * They are not loaded at runtime but must be retained in the final ELF so
 * that llvm-cov can map profile counters back to source lines without
 * requiring the intermediate .a archive files.
 *
 * Using address 0 with no GROUP placement is the standard linker idiom for
 * non-allocatable metadata sections, identical to .debug_info, .comment etc.
 * in include/zephyr/linker/debug-sections.ld.
 *
 * Placed via SECTIONS (snippets-sections.ld), which is outside all memory
 * GROUPs, after GROUP_END(RAMABLE_REGION).
 */
SECTION_PROLOGUE(__llvm_covmap, 0,) { KEEP(*(__llvm_covmap)) }
SECTION_PROLOGUE(__llvm_covfun, 0,) { KEEP(*(__llvm_covfun)) }
