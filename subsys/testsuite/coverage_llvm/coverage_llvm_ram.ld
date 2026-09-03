/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Linker section fragment for LLVM source-based coverage (zero-initialized
 * sections placed in RAM_SECTIONS, before the data copy range).
 *
 * __llvm_prf_cnts (counter arrays) are zero-initialized and must reside in
 * writable RAM. They are placed as NOLOAD (BSS-like) in RAMABLE_REGION.
 *
 * __llvm_prf_bitmap (MC/DC bitmap) is zero-initialized and placed as NOLOAD.
 *
 * Placing these sections in RAM (via RAM_SECTIONS) is critical: without it,
 * the RISC-V PMP marks the text/rodata region as read+execute only, and the
 * very first counter store during early boot causes a Store/AMO access fault.
 */

SECTION_PROLOGUE(__llvm_prf_cnts, (NOLOAD),)
{
	. = ALIGN(8);
	__llvm_prf_cnts_start = .;
	KEEP(*(__llvm_prf_cnts))
	KEEP(*("__llvm_prf_cnts.*"))
	. = ALIGN(8);
	__llvm_prf_cnts_end = .;
} GROUP_NOLOAD_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

SECTION_PROLOGUE(__llvm_prf_bitmap, (NOLOAD),)
{
	. = ALIGN(8);
	__llvm_prf_bitmap_start = .;
	KEEP(*(__llvm_prf_bitmap))
	KEEP(*("__llvm_prf_bitmap.*"))
	. = ALIGN(8);
	__llvm_prf_bitmap_end = .;
} GROUP_NOLOAD_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
