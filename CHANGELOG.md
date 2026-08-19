# Changelog

All notable changes to BINLens are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- GitHub Actions CI workflow running `make fmt-check`, `make`, `make test`, and a `cppcheck` static-analysis pass on every push and pull request.
- `make fmt` and `make fmt-check` targets backed by a checked-in `.clang-format` (LLVM base, 4-space indent, Allman function braces).
- README "Requirements" section documenting the dependency-free toolchain.
- This changelog.

### Changed

- `make test` now stops at the first failing test binary instead of masking earlier failures with the last binary's exit status.
- Applied `clang-format` across `src/`, `include/`, and `tests/` (mechanical reformat, no behavior change).

## [1.0.0] - 2026-07-07

### Added

- Intel HEX parsing for record types `00`, `01`, `02`, `04`, and `05` with checksum validation and line-numbered diagnostics.
- Raw `.bin` loading with configurable base address.
- Address-sorted memory region reconstruction with gap detection.
- Overlap detection with `identical` vs `conflicting` classification.
- Shannon entropy per region and per fixed-size window, with ASCII heatmap.
- ARM Cortex-M vector table detection and confidence scoring.
- Plain text report output.
- Test suite: `test_smoke`, `test_hex_parser`, `test_bin_loader`, `test_memmap`, `test_entropy`, `test_vector_table`.

## [0.1.0] - 2026-07-03

### Added

- Project scaffold: C11, Makefile-based, no dependencies beyond libc.
- Initial Intel HEX parser and diagnostics module.
