/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * LLVM source-based coverage runtime for bare-metal / QEMU targets.
 *
 * This module provides the glue between the LLVM instrumentation runtime
 * (libclang_rt.profile, bare-metal variant) and the Zephyr serial console.
 *
 * Usage:
 *   1. Build with CONFIG_COVERAGE=y and CONFIG_COVERAGE_LLVM_SOURCE=y.
 *   2. At the end of your test, call llvm_coverage_dump().
 *   3. Capture the serial output and extract the hex between
 *      LLVM_PROFILE_DUMP_START and LLVM_PROFILE_DUMP_END.
 *   4. Convert to binary: python3 -c "import sys; sys.stdout.buffer.write(
 *        bytes.fromhex(open('dump.hex').read().strip()))" > default.profraw
 *   5. Process: llvm-profdata merge -o merged.profdata default.profraw
 *      llvm-cov report --instr-profile=merged.profdata zephyr.elf
 *
 * The __llvm_profile_runtime sentinel prevents InstrProfilingRuntime.cpp
 * from registering an atexit() handler that would call
 * __llvm_profile_write_file() (which requires a filesystem).
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <stdint.h>
#ifdef CONFIG_USERSPACE
#include <zephyr/app_memory/app_memdomain.h>
#endif

/*
 * Defining __llvm_profile_runtime suppresses the automatic atexit()
 * registration in InstrProfilingRuntime.cpp that would try to call
 * __llvm_profile_write_file() (unavailable on bare-metal).
 */
int __llvm_profile_runtime;

/*
 * LLVM profile runtime API (from libclang_rt.profile, bare-metal variant).
 * These functions do not perform any OS calls.
 */
extern uint64_t __llvm_profile_get_size_for_buffer(void);
extern int __llvm_profile_write_buffer(char *Buffer);

/*
 * Static buffer for the .profraw data.
 * Size is controlled by CONFIG_LLVM_COVERAGE_PROFILE_BUF_SIZE.
 *
 * This buffer is only written from kernel context (llvm_coverage_dump() is
 * called from kernel/init.c after all tests complete), so it does not need
 * to be in user-accessible memory. Regular BSS (kernel-only RAM) is fine.
 */
static char llvm_profile_buf[CONFIG_LLVM_COVERAGE_PROFILE_BUF_SIZE];

#ifdef CONFIG_USERSPACE
/*
 * When CONFIG_USERSPACE is enabled, user-mode threads need read/write access
 * to LLVM coverage sections:
 *   - __llvm_prf_data: function metadata structs (read by instrumentation)
 *   - __llvm_prf_cnts: coverage counters (written by instrumentation)
 *   - __llvm_prf_bitmap: MC/DC bitmap (written by instrumentation)
 *
 * These sections are placed in RAM by the linker script, but the RISC-V PMP
 * restricts user-mode access to only explicitly granted regions.
 *
 * We register a k_mem_partition covering all three sections and add it to the
 * default memory domain so all user threads can update coverage counters.
 *
 * The linker scripts export the boundary symbols used below.
 */
extern char __llvm_prf_data_start[];
extern char __llvm_prf_data_end[];
extern char __llvm_prf_cnts_start[];
extern char __llvm_prf_cnts_end[];
extern char __llvm_prf_bitmap_start[];
extern char __llvm_prf_bitmap_end[];

static struct k_mem_partition llvm_prf_partition;

static int llvm_coverage_add_partition(void)
{
	/* Cover from __llvm_prf_data_start to __llvm_prf_cnts_end
	 * (and __llvm_prf_bitmap_end if present) as one contiguous region.
	 */
	uintptr_t start = (uintptr_t)__llvm_prf_data_start;
	uintptr_t end = (uintptr_t)__llvm_prf_cnts_end;

	if ((uintptr_t)__llvm_prf_bitmap_end > end) {
		end = (uintptr_t)__llvm_prf_bitmap_end;
	}

	size_t size = end - start;

	if (size == 0) {
		return 0;
	}

	llvm_prf_partition.start = start;
	llvm_prf_partition.size = size;
	llvm_prf_partition.attr = K_MEM_PARTITION_P_RW_U_RW;

	return k_mem_domain_add_partition(&k_mem_domain_default,
					  &llvm_prf_partition);
}

SYS_INIT(llvm_coverage_add_partition, PRE_KERNEL_1, 0);
#endif /* CONFIG_USERSPACE */

/**
 * @brief Dump LLVM coverage profile data to the serial console.
 *
 * Serializes the coverage counters and metadata into a .profraw-format
 * buffer and prints it as a hex string between sentinel markers.
 *
 * Call this function at the end of your test application, after all
 * code under test has executed.
 */
void llvm_coverage_dump(void)
{
	uint64_t size = __llvm_profile_get_size_for_buffer();
	int ret;

	if (size > CONFIG_LLVM_COVERAGE_PROFILE_BUF_SIZE) {
		printk("LLVM_PROFILE_ERROR: buffer too small (%llu > %d)\n",
		       (unsigned long long)size,
		       CONFIG_LLVM_COVERAGE_PROFILE_BUF_SIZE);
		return;
	}

	ret = __llvm_profile_write_buffer(llvm_profile_buf);
	if (ret != 0) {
		printk("LLVM_PROFILE_ERROR: write_buffer failed (%d)\n", ret);
		return;
	}

	printk("LLVM_PROFILE_DUMP_START\n");
	/* Print in chunks of 4096 bytes (8192 hex chars) per line.
	 * This allows the twister serial handler (which uses readline()) to
	 * read each chunk within the timeout window, rather than waiting for
	 * the entire profraw (up to 750KB) as a single line.
	 * gen_profraw_file.py reassembles the chunks into the final .profraw.
	 */
	for (uint64_t i = 0; i < size; i++) {
		printk("%02x", (unsigned char)llvm_profile_buf[i]);
		if ((i + 1) % 4096 == 0 || i + 1 == size) {
			printk("\n");
		}
	}
	printk("LLVM_PROFILE_DUMP_END\n");
}
