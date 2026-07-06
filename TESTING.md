# BINLens Testing

## Standard Workflow

```sh
make clean
make test
make
```

The test suite is plain C and does not require external test frameworks.

## Test Programs

- `test_smoke`: CLI option parsing and basic helpers.
- `test_hex_parser`: Intel HEX records, checksums, EOF handling, and address records.
- `test_bin_loader`: raw binary loading and base-address overflow checks.
- `test_memmap`: adjacent merge, sparse gaps, and overlap classification.
- `test_entropy`: known Shannon entropy values and heatmap symbols.
- `test_vector_table`: Cortex-M vector-table detection and STM32-like end-to-end fixture.

## Fixtures

Fixtures live in `tests/fixtures/` and are intentionally small enough to review by hand.

- `minimal.ihex`
- `extended_linear.ihex`
- `extended_segment.ihex`
- `bad_checksum.ihex`
- `after_eof.ihex`
- `stm32_vector.ihex`

## Manual Validation

Run the STM32-like fixture:

```sh
./binlens --heatmap --entropy-chunk 8 tests/fixtures/stm32_vector.ihex
```

Run a raw binary with a Cortex-M flash base:

```sh
./binlens --format bin --base 0x08000000 firmware.bin
```
