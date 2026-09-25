/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * Linker script fragment for LLVM coverage buffer in app_shmem.
 *
 * When CONFIG_USERSPACE is enabled, the coverage buffer must be placed
 * in app_shmem (user-accessible shared memory) so that user-mode code
 * can update coverage counters without triggering PMP/MPU access faults.
 *
 * This fragment is included via zephyr_linker_sources(RWDATA ...) which
 * places the section in the RWDATA output section (user-accessible).
 */

*(.app_shmem_llvm_profile_buf)
