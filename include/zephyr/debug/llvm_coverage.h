/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DEBUG_LLVM_COVERAGE_H_
#define ZEPHYR_INCLUDE_DEBUG_LLVM_COVERAGE_H_

#ifdef CONFIG_COVERAGE_LLVM_SOURCE
void llvm_coverage_dump(void);
#else
static inline void llvm_coverage_dump(void) { }
#endif /* CONFIG_COVERAGE_LLVM_SOURCE */

#endif /* ZEPHYR_INCLUDE_DEBUG_LLVM_COVERAGE_H_ */
