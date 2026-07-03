# BINLens

Command-line firmware inspection for Intel HEX and raw binary images, focused on STM32-class ARM Cortex-M firmware.

<!-- Badges placeholder: CI, release, coverage, package status -->
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

## Problem Statement

Embedded firmware images are often distributed as `.hex` or `.bin` files with little context about their memory layout. Before flashing, diffing, reverse engineering, or auditing a firmware image, it is useful to answer basic questions:

- Which address ranges are actually populated?
- Are there gaps, overlaps, or suspicious discontinuities?
- Does the image contain likely vector tables for ARM Cortex-M targets?
- Which regions look like code, padding, compressed data, or encrypted data?

BINLens exists to provide that first-pass inspection from the command line with no external dependencies. The target use case is practical STM32 firmware work, not full disassembly or reverse engineering.

## Features

### MVP Features

- Parse Intel HEX files with data, EOF, extended segment address, extended linear address, and start linear address records.
- Validate Intel HEX checksums and report line-specific parse errors.
- Load raw `.bin` files with a configurable base address.
- Reconstruct populated memory regions from sparse input data.
- Detect and report gaps and overlaps between records or reconstructed regions.
- Compute Shannon entropy per region and per fixed-size chunk.
- Detect likely ARM Cortex-M vector tables at common flash aliases such as `0x00000000` and `0x08000000`.
- Print concise text summaries suitable for terminals and CI logs.
- Build with a C11 compiler and Makefile, using only the standard C library.

### Stretch Features

- ASCII entropy heatmap for quick visual scanning.
- Opcode-density heuristics for identifying likely Thumb code regions.
- Section boundary guessing based on entropy, padding, and alignment patterns.
- JSON output for scripts and automated firmware comparison.
- Side-by-side comparison of two firmware images.
- Optional symbol/map-file correlation.

## Usage

Inspect an Intel HEX image:

```sh
binlens firmware.hex
```

Inspect a raw binary image and specify the flash base address:

```sh
binlens --base 0x08000000 firmware.bin
```

Use a smaller entropy chunk size when looking for short high-entropy blocks:

```sh
binlens --entropy-chunk 256 --heatmap firmware.hex
```

Example output:

```text
$ binlens --heatmap build/app.hex
BINLens 0.1.0

Input
  File:          build/app.hex
  Format:        Intel HEX
  Records:       1842
  Address span:  0x08000000 - 0x0801B7FF
  Bytes loaded:  98304

ARM Cortex-M Vector Table
  Candidate:     0x08000000
  Initial SP:    0x20010000  valid SRAM range
  Reset handler: 0x080013C1  valid Thumb flash address
  Confidence:    high

Memory Regions
  #  Start       End         Size       Entropy  Notes
  0  0x08000000  0x08003FFF  16.0 KiB   5.91    vectors + code
  1  0x08004000  0x08017BFF  79.0 KiB   7.72    high entropy
  2  0x08018000  0x0801B7FF  14.0 KiB   0.18    likely padding

Gaps
  0x08017C00 - 0x08017FFF  1.0 KiB

Overlaps
  none

Entropy Heatmap, chunk=1024 bytes
  0x08000000  ..::--==++++****####%%%%
  0x08008000  %%%%%%%%%%%%%%%%%%%%%%%%
  0x08010000  %%%%%%%%%%%%%%%%####**++
  0x08018000  ................

Legend
  . 0.0-1.0   : 1.0-2.5   - 2.5-4.0   = 4.0-5.5
  + 5.5-6.5   * 6.5-7.2   # 7.2-7.8   % 7.8-8.0
```

Example warning output for an invalid HEX file:

```text
$ binlens broken.hex
error: Intel HEX checksum mismatch on line 42
       expected 0xA7, computed 0x9C
```

The heatmap uses fixed-size chunks because a stable window makes entropy changes easier to compare across files and runs.

## Installation

Build from source:

```sh
make
```

Run the binary from the build directory:

```sh
./binlens --help
```

Install system-wide:

```sh
make install
```

Remove build artifacts:

```sh
make clean
```

The project is intended to build on POSIX-compatible systems such as Linux and macOS with a C11 compiler. No libraries beyond the standard C library are required.

## Supported Input Formats

- Intel HEX (`.hex`, `.ihex`)
  - Data records
  - End-of-file records
  - Extended segment address records
  - Extended linear address records
  - Start linear address records
  - Checksum validation
- Raw binary (`.bin`)
  - Loaded as one contiguous region
  - Base address defaults to `0x00000000`
  - Base address can be overridden with `--base`

## License

BINLens is released under the MIT License. See [LICENSE](LICENSE).
