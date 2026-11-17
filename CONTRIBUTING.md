# Contributing

## Development environment

A [Nix](https://nixos.org) package, flake, and development shell are provided
for reproducible builds and tooling.

Build and run the packaged binary:

```bash
nix build .#     # Builds into result/bin/tomato
nix run .#       # Runs the binary
nix build .#tcc  # Builds with TinyCC
nix build .#zig  # Builds with Zig's C compiler
nix flake check  # Builds and tests GCC, TinyCC, and Zig variants
```

Enter the development shell:

```bash
nix develop # Flake-based shell
nix-shell   # Classic, non-flake shell
```

The development shell provides GCC, Make, pkg-config, ncurses, libnotify,
clang-format, Bear, Doxygen, TinyCC, Zig, and Valgrind.

It also keeps Tomato.C's configuration, state, and runtime files under `.tmp/`
in the repository through `TOMATO_CONFIG_DIR`, `TOMATO_STATE_DIR`, and
`TOMATO_RUNTIME_DIR`. This prevents development builds from modifying your
actual user configuration. The shell does not override the `XDG_*` environment
variables.

Build and run tests from inside the development shell:

```bash
./build.sh
./run_tests.sh
```

## Build and test

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

Add or update tests when changing behavior. Include coverage for relevant edge
cases, invalid inputs, and failure paths.

## Formatting, naming, and documentation

Format changed C headers and sources with the repository configuration:

```bash
clang-format -i src/changed_file.c src/changed_file.h
```

Every header file must use include guards following the `FILENAME_H_` convention:

```c
#ifndef FILENAME_H_
#define FILENAME_H_

/* Header contents */

#endif /* FILENAME_H_ */
```

Use the filename in uppercase, replacing characters unsuitable for the guard
with underscores. Keep the opening and closing guard names identical.

Follow these naming conventions:

- **Variables and struct fields:** `snake_case`.
- **Constants, `#define` macros, and enum members:** `UPPER_SNAKE_CASE`.
- **Private functions:** `camelCase`.
- **Public functions:** `PascalCase`.
- **Private structs and enums:** `camelCase`.
- **Public structs and enums:** `PascalCase`.

Keep functions and types file-local when they are only used within their own
`.c` file:

- Declare private functions as `static`.
- Place private function declarations below the `/* PRIVATE FILENAME FUNCTIONS */`
  comment.
- Place private function implementations immediately below the function that
  calls them.
- Public functions must have external linkage and be declared in their
  corresponding header.
- Apply the same private/public naming distinction to structs and enums.

Use Doxygen-style documentation consistently:

- **Functions:** Use a `/** ... */` block immediately before the declaration or
  definition. Document the function's purpose, parameters with `@param`, and
  return value with `@return`. Document ownership and lifetime requirements
  where relevant.
- **Structs and enums:** Document the type with a Doxygen block and each field
  or enum member with an inline `/**< COMMENT */` annotation.
- **Private functions:** Document non-obvious behavior and relevant parameters
  or return values; trivial helpers do not require extensive documentation.

Prefer `ErrorType` as the return type for functions that can fail, rather than
using generic integers or unrelated types. Return an appropriate error code on
failure and `NO_ERROR` on success whenever applicable.

For example:

```c
/**
 * Send a notification with sound using a Notification struct.
 * @param notification Pointer to the notification data.
 * @return NO_ERROR when sent or queued, or an error code on validation or
 *         dispatch failure.
 */
ErrorType SendNotification(const Notification *notification);

/**
 * Notification data structure containing notification content.
 * Used to send system notifications to the user.
 */
typedef struct {
  const char* title;       /**< Notification title */
  const char* description; /**< Detailed description text */
  const char* audio_path;  /**< Path to audio file to play (or NULL) */
} Notification;
```

Generate API documentation with:

```bash
cd docs
./generate.sh
```

## Implementation guidelines

- **Error handling:** Check return values and propagate or handle errors
  appropriately. Avoid silently ignoring failures.
- **Memory management:** Make ownership and lifetime responsibilities clear.
  Release allocated resources and avoid memory leaks, double frees, and
  use-after-free errors.
- **Header dependencies:** Include only what is required. Use forward
  declarations where appropriate, and ensure headers can be included
  independently.
- **Scope:** Keep changes focused and avoid introducing unrelated
  modifications.

## Changes and commits

- Add or update tests for behavior changes.
- Run relevant tests and `git diff --check` before committing.
- Use [Conventional Commits](https://www.conventionalcommits.org/en/v1.0.0),
  with a specific subject and a body describing behavior and tests.
- Keep changes focused. Update `README.md`, `ARCHITECTURE.md`, or
  `sample_config.toml` when user-visible behavior, architecture, or
  configuration changes.
