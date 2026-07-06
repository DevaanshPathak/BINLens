# Contributing to BINLens

BINLens is initially a solo project, but issues, bug reports, test cases, and focused patches are welcome.

## Coding Style

- Use C11.
- Keep the project portable across POSIX-compatible Linux and macOS systems.
- Use only the standard C library for v1.
- Prefer small modules with clear ownership over broad utility files.
- Use `snake_case` for functions and variables.
- Use `UpperCamelCase` only for type names if the codebase adopts typedef-style names.
- Keep error handling explicit and return actionable diagnostics.
- Avoid compiler-specific extensions unless they are guarded and justified.

## Build

```sh
make
```

Run the CLI locally:

```sh
./binlens --help
```

Clean generated files:

```sh
make clean
```

## Tests

The test workflow should stay Makefile-based:

```sh
make test
```

See [TESTING.md](TESTING.md) for the current fixture set and manual validation commands.

Tests should cover:

- Valid Intel HEX files.
- Invalid checksums and malformed HEX records.
- Raw binary base-address handling.
- Sparse regions and gaps.
- Overlapping and conflicting ranges.
- Entropy known values.
- Cortex-M vector table detection.

## Issues and Pull Requests

When opening an issue, include:

- BINLens version or commit.
- Operating system and compiler.
- Input format.
- Minimal firmware fixture or a small synthetic reproducer when possible.
- Expected result and actual result.

Pull requests should be focused and should include tests when behavior changes. Large design changes should start as an issue so the scope is clear before implementation work begins.
