# BINLens Architecture

## Module Breakdown

- `main.c`: Program entry point, top-level error handling, and process exit codes.
- `cli.c` / `cli.h`: Command-line option parsing and validation.
- `hex_parser.c` / `hex_parser.h`: Intel HEX parsing, checksum validation, address-base handling, and parse diagnostics.
- `bin_loader.c` / `bin_loader.h`: Raw binary loading with configurable base address.
- `firmware_image.c` / `firmware_image.h`: Shared representation of loaded chunks, reconstructed regions, and input metadata.
- `memmap.c` / `memmap.h`: Address sorting, adjacent-range merging, gap detection, and overlap detection.
- `entropy.c` / `entropy.h`: Shannon entropy calculation for regions and fixed-size chunks.
- `vector_table.c` / `vector_table.h`: ARM Cortex-M vector table candidate detection and confidence scoring.
- `format.c` / `format.h`: Human-readable sizes, addresses, tables, diagnostics, and optional heatmap formatting.
- `diagnostic.c` / `diagnostic.h`: Structured error and warning messages.
- `tests/`: Small fixture-based parser, memory map, entropy, and vector table tests.

This split keeps parsing, reconstruction, analysis, and presentation separate so each part can be tested without invoking the full CLI.

## Core Data Structures

The following sketches describe the intended shape of the data model. They are not compilable C definitions.

### Firmware Image

```text
FirmwareImage
  source_path
  input_format
  chunks: list of source chunks exactly as loaded from HEX records or BIN data
  regions: list of reconstructed contiguous memory regions
  gaps: list of address gaps between regions
  overlaps: list of overlap diagnostics
  metadata:
    record_count
    eof_seen
    start_linear_address, if present
    total_loaded_bytes
    address_span_start
    address_span_end
```

`FirmwareImage` is the central object passed from loaders into analysis. It stores original chunks as well as reconstructed regions because overlap detection needs source-level detail.

### Source Chunk

```text
SourceChunk
  start_address
  length
  bytes
  origin:
    input line number for HEX, or file offset for BIN
```

A source chunk represents one loaded block before merging. Keeping origin information makes parse and overlap diagnostics actionable.

### Memory Region

```text
MemRegion
  start_address
  end_address
  length
  bytes
  entropy
  notes
```

A memory region represents contiguous populated memory after sorting and merging source chunks. Gaps are not inserted as synthetic zero bytes in v1 because that would distort entropy results.

### Gap

```text
MemGap
  start_address
  end_address
  length
```

Gaps describe missing address ranges between reconstructed regions.

### Overlap

```text
MemOverlap
  start_address
  end_address
  length
  first_origin
  second_origin
  byte_status: identical or conflicting
```

Overlaps are reported separately from reconstructed regions because overlapping HEX records may be legal in some toolchains but still deserve visibility.

### Entropy Chunk

```text
EntropyChunk
  start_address
  end_address
  length
  entropy
  heatmap_symbol
```

Entropy chunks are derived from memory regions using a fixed window size. The default window is 1024 bytes because it balances local detail against noisy byte-by-byte variation.

### Vector Table Candidate

```text
VectorTableCandidate
  table_address
  initial_stack_pointer
  reset_handler_raw
  reset_handler_address
  stack_pointer_validity
  reset_handler_validity
  confidence
  notes
```

The reset handler is stored both raw and with the Thumb bit cleared so the report can show exactly what was encoded in the firmware image.

## High-Level Data Flow

```text
input file
  -> CLI resolves format and options
  -> HEX parser or BIN loader produces source chunks
  -> memory map builder sorts chunks by absolute address
  -> overlap detector compares intersecting source chunks
  -> region builder merges adjacent compatible chunks
  -> gap detector records missing ranges between regions
  -> entropy analyzer computes per-region and per-chunk entropy
  -> vector table detector checks common Cortex-M locations and region starts
  -> formatter prints summary, tables, warnings, and optional heatmap
```

## Design Notes

- Use 64-bit unsigned address arithmetic internally to avoid overflow on large sparse address ranges.
- Keep byte buffers owned by the firmware image so analysis modules can reference stable memory.
- Treat parse errors as fatal and analysis warnings as non-fatal.
- Do not fill gaps by default; missing bytes and zero bytes are different facts.
- Keep terminal output deterministic by sorting all address-based collections before formatting.
- Make parser and analysis modules independent from terminal formatting so tests can assert structured results.
