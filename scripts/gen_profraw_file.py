#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2024 Qualcomm Innovation Center, Inc.
"""
Parse a serial console log file and reconstruct the .profraw binary file
produced by LLVM source-based coverage instrumentation.

The firmware must be built with CONFIG_COVERAGE=y and CONFIG_COVERAGE_LLVM_SOURCE=y.
The serial output contains a hex-encoded .profraw dump between the markers:

  LLVM_PROFILE_DUMP_START
  <hex string>
  LLVM_PROFILE_DUMP_END

Usage:
    python3 scripts/gen_profraw_file.py -i handler.log -o default.profraw
    llvm-profdata merge -sparse -o merged.profdata default.profraw
    llvm-cov report --instr-profile=merged.profdata build/app/libapp.a
"""

import argparse
import re
import sys


def retrieve_data(input_file):
    """Parse LLVM_PROFILE_DUMP_START/END markers from serial log.

    Returns the hex string between the markers, or None if not found.
    The hex data may span multiple lines (chunked output from firmware).
    """
    hex_chunks = []
    capture = False
    complete = False
    with open(input_file) as fp:
        for line in fp.readlines():
            if re.search("LLVM_PROFILE_DUMP_START", line):
                capture = True
                continue
            if re.search("LLVM_PROFILE_DUMP_END", line):
                complete = True
                break
            if capture:
                chunk = line.strip()
                if chunk:
                    hex_chunks.append(chunk)

    if not complete:
        print(f"Warning: incomplete profile data in {input_file}", file=sys.stderr)
    return ''.join(hex_chunks) if hex_chunks else None


def create_profraw_file(hex_data, output_file, verbose=False):
    """Write binary .profraw from hex string."""
    if not hex_data:
        print("Error: no profile data found", file=sys.stderr)
        sys.exit(1)
    try:
        data = bytes.fromhex(hex_data)
    except ValueError as e:
        print(f"Error: invalid hex data: {e}", file=sys.stderr)
        sys.exit(1)
    with open(output_file, 'wb') as fp:
        fp.write(data)
    if verbose:
        print(f"Written: {output_file} ({len(data)} bytes)")


def parse_args():
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("-i", "--input", required=True,
                        help="Serial log file containing LLVM profile dump")
    parser.add_argument("-o", "--output", default="default.profraw",
                        help="Output .profraw file (default: %(default)s)")
    parser.add_argument("-v", "--verbose", action="store_true",
                        help="Enable verbose output")
    return parser.parse_args()


def main():
    args = parse_args()
    hex_data = retrieve_data(args.input)
    create_profraw_file(hex_data, args.output, args.verbose)


if __name__ == "__main__":
    main()
