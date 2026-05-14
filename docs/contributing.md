# Contributing

## Repository

https://github.com/VersaNexusIX/vpi

## Getting Started

```sh
git clone https://github.com/VersaNexusIX/vpi
cd vpi
make
./vpi help
```

## Project Structure

```
vpi/
  src/
    main.c          Entry point and command routing
    vpi.h / vpi.c   Core types, init, and platform path resolution
    commands.h / .c Command implementations
    cmd_init.c      Interactive vers.json generator
    http.c          HTTP download via libcurl
    zip.c           ZIP extraction wrapper
    miniz.h / .c    Self-contained ZIP and inflate implementation
    registry.h / .c Local installed package database
    utils.h / .c    String, file, path, JSON, and registry index utilities
  docs/             Documentation
  scripts/          Install scripts and package helper
  examples/         Example vers.json
```

## Code Style

- C11 standard throughout.
- No memory leaks: every allocation has a corresponding free on all paths.
- No commented-out code, no unnecessary blank lines.
- Functions are static unless declared in a header.
- Error output goes to `stderr`. Normal output goes to `stdout`.
- All paths constructed with `path_join()`.
- No global mutable state outside of `vpi.c` and `utils.c`.

## Adding a Command

1. Add the declaration to `src/commands.h`.
2. Implement it in `src/commands.c` or a new `src/cmd_<name>.c`.
3. Add the source file to `SRCS` in `Makefile` and `VPI_SOURCES` in `CMakeLists.txt`.
4. Wire the command in the `main()` router in `src/main.c`.
5. Add it to `print_help()` in `src/main.c`.
6. Document it in `docs/cli-reference.md`.

## Registry Index

VPI resolves packages through `versas.my.id/vers.json` before downloading. The lookup function is `registry_index_lookup()` in `src/utils.c`. If the index is unreachable, VPI falls back to a direct URL. Keep this fallback in place.

## Reporting Issues

Open an issue at https://github.com/VersaNexusIX/vpi/issues.

## License

By contributing, you agree that your contributions will be licensed under the NeoDAX License 1.0 (Origin). See [LICENSE](../LICENSE).
