/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc.
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
 * The buffer is placed in BSS (zero-initialized, writable RAM).
 */
static char llvm_profile_buf[CONFIG_LLVM_COVERAGE_PROFILE_BUF_SIZE];

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
