/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Linker section fragment for LLVM source-based coverage (initialized data
 * sections placed in DATA_SECTIONS, within the data copy range).
 *
 * __llvm_prf_data (function metadata structs) are initialized data containing
 * counter pointer offsets. They must reside in writable RAM (because the
 * CounterPtr field is a relative offset to the counter array in RAM), and
 * must be loaded from ROM (because they contain compile-time-initialized
 * values).
 *
 * Placing this section in DATA_SECTIONS ensures it is within the
 * [__data_region_start, __data_region_end] range that the Zephyr boot code
 * copies from ROM to RAM.
 */

SECTION_DATA_PROLOGUE(__llvm_prf_data,,)
{
	. = ALIGN(8);
	__llvm_prf_data_start = .;
	KEEP(*(__llvm_prf_data))
	KEEP(*("__llvm_prf_data.*"))
	. = ALIGN(8);
	__llvm_prf_data_end = .;
} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
