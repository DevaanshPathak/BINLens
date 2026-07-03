# BINLens Product Requirements Document

## Problem Statement

Firmware images for STM32-class embedded systems are usually inspected with generic tools such as hex dumpers, objdump, or vendor flash utilities. Those tools are useful, but they do not provide a compact firmware-oriented view of sparse address ranges, gaps, overlaps, vector table candidates, and entropy patterns.

BINLens should provide a fast first-pass command-line inspection tool for C developers working with ARM Cortex-M firmware images.

## Target User

The primary user is an embedded C developer comfortable with memory maps, linker scripts, flash addresses, Intel HEX records, and ARM Cortex-M boot behavior. The tool should be useful during local STM32 development, firmware intake, CI checks, and lightweight reverse engineering triage.

## Goals

- Parse Intel HEX and raw binary firmware files reliably.
- Reconstruct populated memory ranges in address order.
- Report region sizes, start/end addresses, gaps, and overlaps.
- Compute Shannon entropy at region and chunk granularity.
- Detect likely ARM Cortex-M vector tables as a first-class feature.
- Provide terminal output that is readable without post-processing.
- Stay portable, dependency-free, and small enough for a 4-week solo project.

## Non-Goals for v1

- No disassembler.
- No instruction-level control-flow analysis.
- No ELF parsing.
- No automatic MCU database or part-number detection.
- No flashing, debugging, or SWD/JTAG integration.
- No firmware unpacking, decryption, or decompression.
- No GUI or TUI.
- No dependency on libelf, libmagic, OpenSSL, zlib, or vendor SDKs.
- No guarantee that entropy alone identifies encryption or compression; it is a heuristic signal only.

## Functional Requirements

### A. Intel HEX Parsing

BINLens must parse Intel HEX records according to the common Intel HEX format:

- Record type `00`: data record.
- Record type `01`: end-of-file record.
- Record type `02`: extended segment address.
- Record type `04`: extended linear address.
- Record type `05`: start linear address.

Requirements:

- Validate the leading colon, byte count, address, record type, data bytes, and checksum.
- Reject malformed hexadecimal characters.
- Validate every record checksum and report the failing line number.
- Track the active address base from type `02` and type `04` records.
- Convert each data record into an absolute address range.
- Preserve enough metadata to detect overlaps caused by repeated or conflicting data records.
- Treat data after EOF as an error by default.
- Parse type `05` and report the start linear address as metadata, but do not treat it as a memory region.
- Report unsupported record types with a clear diagnostic.

Design decision: checksum validation is mandatory because firmware ingestion should fail early when a transfer or export is corrupt.

### B. Raw `.bin` Loading

BINLens must load raw binary files as one contiguous memory region.

Requirements:

- Accept `.bin` input by extension or explicit `--format bin`.
- Default the base address to `0x00000000`.
- Allow the user to specify a base address with `--base 0x...`.
- Store file offset to address mapping as `address = base + offset`.
- Reject base addresses or sizes that overflow the internal address type.

Design decision: raw binaries have no embedded address metadata, so the base address must be explicit for meaningful Cortex-M analysis.

### C. Memory Region Reconstruction

BINLens must reconstruct a firmware image from parsed address ranges.

Requirements:

- Normalize parsed data chunks into address-sorted ranges.
- Merge adjacent chunks when `end + 1 == next_start`.
- Detect gaps between non-adjacent ranges.
- Detect overlaps between source chunks before or during merge.
- Distinguish identical overlapping bytes from conflicting overlapping bytes when possible.
- Preserve per-region byte data for entropy analysis.
- Report total loaded bytes and total address span.

Design decision: merging adjacent ranges gives a useful firmware-level view while still allowing overlap diagnostics from the original input chunks.

### D. Entropy Analysis

BINLens must compute Shannon entropy in bits per byte.

Requirements:

- Compute entropy over each reconstructed region.
- Compute entropy over fixed-size chunks for heatmap and detailed output.
- Default chunk size: 1024 bytes.
- Allow configurable chunk size with `--entropy-chunk`.
- Handle partial final chunks.
- Report entropy from `0.00` to `8.00`.
- Avoid treating missing gaps as zero-filled data unless the user explicitly asks for a padded span view in a later version.

Design decision: 1024 bytes is large enough to smooth tiny local noise but small enough to reveal practical firmware boundaries and padding regions.

### E. ARM Cortex-M Vector Table Detection

BINLens must detect likely ARM Cortex-M vector tables.

Requirements:

- Search at the start of reconstructed regions and common aliases including `0x00000000` and `0x08000000`.
- Read the first two little-endian 32-bit words as initial stack pointer and reset handler.
- Validate initial stack pointer against common SRAM ranges, especially `0x20000000` through `0x3FFFFFFF`.
- Validate reset handler as a Thumb address by checking bit 0 is set.
- Validate reset handler target after clearing bit 0 against populated executable-looking flash ranges when possible.
- Report candidate address, initial stack pointer, reset handler, validation notes, and confidence.
- Do not fail the whole analysis when no vector table is found.

Design decision: vector table detection is a primary workflow because STM32 firmware inspection often begins by confirming the image base and reset entry point.

### F. CLI Interface and Output Formatting

BINLens must provide a simple command-line interface.

Required options:

- `binlens <file>`
- `--format auto|hex|bin`
- `--base <address>`
- `--entropy-chunk <bytes>`
- `--heatmap`
- `--no-color`
- `--verbose`
- `--help`
- `--version`

Output requirements:

- Print input summary, reconstructed regions, gaps, overlaps, entropy, and vector table results.
- Use deterministic ordering by address.
- Use hexadecimal addresses with a `0x` prefix and fixed width where practical.
- Use human-readable sizes such as `KiB` for summaries.
- Keep default output concise enough for terminal use.
- Print errors to standard error and return non-zero on parse or validation failure.

Design decision: plain text is the default because the v1 audience is a developer working in terminals and build logs.

## Non-Functional Requirements

- Language: C11.
- Platform: POSIX-compatible Linux and macOS.
- Build system: Makefile.
- Dependencies: standard C library only.
- Performance: handle firmware images of at least 64 MiB within a few seconds on a typical development machine.
- Memory use: proportional to loaded firmware bytes plus parser metadata.
- Determinism: same input and options must produce the same output.
- Robustness: malformed input must produce clear diagnostics rather than undefined behavior.
- Portability: avoid compiler-specific extensions in v1.
- Testability: parser, memory map, entropy, and vector table detection should be separately testable.

## Success Criteria for v1

- Correctly parses valid Intel HEX files using record types `00`, `01`, `02`, `04`, and `05`.
- Rejects Intel HEX files with invalid checksums or malformed records.
- Loads raw `.bin` files with default and explicit base addresses.
- Produces accurate reconstructed region, gap, and overlap reports.
- Computes entropy values that match known test vectors.
- Detects a valid STM32-style vector table at `0x08000000`.
- Provides useful diagnostics for missing or suspicious vector tables.
- Builds with `make` on Linux and macOS using a C11 compiler.
- Includes a small test corpus covering valid, malformed, sparse, overlapping, and high-entropy inputs.
- README usage examples match the implemented CLI behavior.
