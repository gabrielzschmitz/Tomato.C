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

## Formatting, naming, and documentation

Format changed C headers and sources with the repository configuration:

```bash
clang-format -i src/changed_file.c src/changed_file.h
```

Every header file must use include guards following the `FILENAME_H_`
convention:

```c
#ifndef FILENAME_H_
#define FILENAME_H_

/* Header contents */

#endif /* FILENAME_H_ */
```

Use the filename in uppercase, replacing any characters unsuitable for the
guard with underscores. Keep the opening and closing guard names identical.

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
- Place private function declarations below the `/* PRIVATE FILENAME FUNCTIONS
*/` comment.
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

## Changes and commits

- Add or update tests for behavior changes.
- Run relevant tests and `git diff --check` before committing.
- Use [Conventional Commits](https://www.conventionalcommits.org/en/v1.0.0),
  with a specific subject and a body describing behavior and tests.
- Keep changes focused; update `README.md`, `ARCHITECTURE.md`, or
  `sample_config.toml` when user-visible behavior or configuration changes.
