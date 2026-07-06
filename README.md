# BINLens

Command-line firmware inspection for Intel HEX and raw binary images, focused on STM32-class ARM Cortex-M firmware.

[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Version](https://img.shields.io/badge/version-1.0.0-green.svg)](#)

## Why It Exists

Firmware images are often handed around as `.hex` or `.bin` files with little context. Before flashing, comparing, or auditing one, it helps to answer:

- Which address ranges are populated?
- Are there gaps or overlapping records?
- Does the image look like Cortex-M firmware?
- Where are high-entropy or padding-like regions?

BINLens gives that first-pass inspection from a terminal with a small C11 codebase and no dependencies beyond the standard C library.

## Features

### v1 MVP

- Intel HEX parsing for record types `00`, `01`, `02`, `04`, and `05`.
- Intel HEX checksum validation with line-numbered diagnostics.
- Raw `.bin` loading with configurable base address.
- Address-sorted memory region reconstruction.
- Gap detection between reconstructed regions.
- Overlap detection with `identical` vs `conflicting` classification.
- Shannon entropy per region.
- Fixed-size entropy windows with configurable chunk size.
- ASCII entropy heatmap.
- ARM Cortex-M vector table detection and confidence scoring.
- Plain text output suitable for terminals and CI logs.

### Stretch Ideas

- JSON output.
- Firmware-to-firmware comparison.
- Thumb opcode-density heuristics.
- Section boundary guessing.
- Optional map/symbol correlation.

## Usage

Inspect an Intel HEX image:

```sh
binlens firmware.hex
```

Inspect a raw binary at an STM32 flash base:

```sh
binlens --format bin --base 0x08000000 firmware.bin
```

Show chunk entropy and heatmap:

```sh
binlens -v --heatmap --entropy-chunk 1024 firmware.hex
```

Example output:

```text
$ binlens --heatmap --entropy-chunk 8 tests/fixtures/stm32_vector.ihex
BINLens 1.0.0

Input
  File:          tests/fixtures/stm32_vector.ihex
  Format:        Intel HEX
  Records:       5
  Data records:  2
  Bytes loaded:  20 (20 B)
  Source chunks: 2
  Address span:  0x08000000 - 0x08000103
  Start linear:  0x08000101

ARM Cortex-M Vector Table
  Candidate:     0x08000000
  Initial SP:    0x20001000  valid SRAM range, aligned
  Reset handler: 0x08000101  target 0x08000100, Thumb bit set, populated target
  Confidence:    high (score 8)

Memory Regions
  #  Start       End         Size     Entropy
  0  0x08000000  0x0800000F  16 B     1.50
  1  0x08000100  0x08000103  4 B      1.00

Gaps
  #  Start       End         Size
  0  0x08000010  0x080000FF  240 B

Overlaps
  none

Entropy Heatmap, chunk=8 bytes
  0x08000000  :.
  0x08000100  :
```

## Build

```sh
make
make test
```

Run locally:

```sh
./binlens --help
```

Install:

```sh
make install
```

Clean:

```sh
make clean
```

The Makefile targets POSIX-compatible Linux/macOS environments. Windows builds are currently produced with MSYS2 GCC.

## Supported Formats

- Intel HEX: `.hex`, `.ihex`
- Raw binary: `.bin`

Raw binaries default to base address `0x00000000`; use `--base` for STM32 flash images.

## License

MIT. See [LICENSE](LICENSE).
