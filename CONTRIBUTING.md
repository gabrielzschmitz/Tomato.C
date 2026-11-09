# Contributing

## Build and test

Use the Nix development shell for consistent dependencies and tools:

```bash
nix-shell
./build.sh
./run_tests.sh
```

Run the full test suite with AddressSanitizer and UndefinedBehaviorSanitizer:

```bash
./run_tests.sh --asan
```

The lower-level test Makefile also accepts `ASAN=1`:

```bash
make -C tests clean
make -C tests ASAN=1 build
```

Run focused tests with `./run_tests.sh --filter log` or
`./run_tests.sh --filter notify`. Run `./run_tests.sh --ubsan` for a
standalone UndefinedBehaviorSanitizer pass.

## Formatting and documentation

Format changed C headers and sources with the repository configuration:

```bash
clang-format -i src/changed_file.c src/changed_file.h
```

Use Doxygen-style comments for public APIs and non-obvious private functions.
Document parameters, return values, and ownership where relevant. Keep
file-local helpers `static`; declare cross-file APIs in their header.

Generate API documentation with:

```bash
cd docs
./generate.sh
```

## Changes and commits

- Add or update tests for behavior changes.
- Run relevant tests and `git diff --check` before committing.
- Use [Conventional Commits](https://www.conventionalcommits.org/en/v1.0.0),
  with a specific subject and a body describing behavior and tests.
- Keep changes focused; update `README.md`, `ARCHITECTURE.md`, or
  `sample_config.toml` when user-visible behavior or configuration changes.
