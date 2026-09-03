/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Linker section fragment for LLVM source-based coverage (read-only sections).
 *
 * Creates a dedicated output section for __llvm_prf_names in the
 * ROMABLE_REGION (ROM/flash).
 *
 * Using a dedicated output section (rather than merging into 'rodata') is
 * required so that the linker generates the __start___llvm_prf_names and
 * __stop___llvm_prf_names boundary symbols that libclang_rt.profile uses
 * to locate the names data.
 *
 * __llvm_covmap and __llvm_covfun are non-allocatable COMDAT sections used
 * only by host tools (llvm-cov). They are left as orphan sections and will
 * be merged into rodata by the linker.
 *
 * Placed via ROM_SECTIONS (snippets-rom-sections.ld), which is included
 * before the rodata section in the ROMABLE_REGION GROUP.
 */

SECTION_PROLOGUE(__llvm_prf_names, , ALIGN(8))
{
	KEEP(*(__llvm_prf_names))
	KEEP(*("__llvm_prf_names.*"))
	. = ALIGN(8);
} GROUP_LINK_IN(ROMABLE_REGION)
